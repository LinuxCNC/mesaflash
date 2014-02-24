//
//    Copyright (C) 2013-2014 Michael Geszkiewicz
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "eeprom.h"
#include "eeprom_remote.h"
#include "eth_boards.h"
#include "lbp16.h"
#include "bitfile.h"

extern u8 boot_block[BOOT_BLOCK_SIZE];
extern u8 page_buffer[PAGE_SIZE];
extern u8 file_buffer[SECTOR_SIZE];

static void read_page(llio_t *self, u32 addr, void *buff) {
    lbp16_cmd_addr_data32 write_addr_pck;
    lbp16_cmd_addr read_page_pck;
    int send, recv;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    LBP16_INIT_PACKET4(read_page_pck, CMD_READ_FPGA_FLASH_ADDR32(64), FLASH_DATA_REG);

    send = eth_socket_send_packet(&write_addr_pck, sizeof(write_addr_pck));
    send = eth_socket_send_packet(&read_page_pck, sizeof(read_page_pck));
    recv = eth_socket_recv_packet(buff, PAGE_SIZE);
}

static void write_page(llio_t *self, void *buff) {
    lbp16_write_flash_page_packets write_page_pck;
    int send, recv;
    u32 ignored;

    LBP16_INIT_PACKET6(write_page_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A03);
    LBP16_INIT_PACKET4(write_page_pck.fl_write_page_pck, CMD_WRITE_FPGA_FLASH_ADDR32(64), FLASH_DATA_REG);
    memcpy(&write_page_pck.fl_write_page_pck.page, buff, 256);
    send = eth_socket_send_packet(&write_page_pck, sizeof(write_page_pck));
    // packet read for board syncing
    recv = lbp16_read(CMD_READ_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, &ignored, 4);
}

static void erase_sector(llio_t *self, u32 addr) {
    lbp16_erase_flash_sector_packets sector_erase_pck;
    lbp16_cmd_addr_data32 write_addr_pck;
    int send, recv;
    u32 ignored;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    send = eth_socket_send_packet(&write_addr_pck, sizeof(write_addr_pck));
    if (send < 0)
        printf("ERROR: %s(): %s\n", __func__, strerror(errno));

    LBP16_INIT_PACKET6(sector_erase_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A03);
    LBP16_INIT_PACKET8(sector_erase_pck.fl_erase_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_SEC_ERASE_REG, 0);
    send = eth_socket_send_packet(&sector_erase_pck, sizeof(sector_erase_pck));
    if (send < 0)
        printf("ERROR: %s(): %s\n", __func__, strerror(errno));
    // packet read for board syncing
    recv = lbp16_read(CMD_READ_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, &ignored, 4);
}

static int check_boot(llio_t *self) {
    board_t *board = self->private;
    int i;

// if board doesn't support fallback there is no boot block
    if (board->fallback_support == 0)
        return 0;

    read_page(self, 0x0, &page_buffer);
    for (i = 0; i < BOOT_BLOCK_SIZE; i++) {
        if (boot_block[i] != page_buffer[i]) {
            return -1;
        }
    }
    return 0;
}

static void write_boot(llio_t *self) {
    printf("Erasing sector 0 for boot block\n");
    erase_sector(self, BOOT_ADDRESS);
    memset(&file_buffer, 0, PAGE_SIZE);
    memcpy(&file_buffer, &boot_block, BOOT_BLOCK_SIZE);
    write_page(self, file_buffer);
    printf("BootBlock installed\n");
}

static void write_flash_address(llio_t *self, u32 addr) {
    lbp16_cmd_addr_data32 write_addr_pck;
    int send;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    send = eth_socket_send_packet(&write_addr_pck, sizeof(write_addr_pck));
}

static int start_programming(llio_t *self, u32 start_address, int fsize) {
    board_t *board = self->private;
    u32 sec_addr;
    int esectors, sector, max_sectors;

    esectors = (fsize - 1) / SECTOR_SIZE;
    if (start_address == FALLBACK_ADDRESS) {
        max_sectors = eeprom_calc_user_space(board->flash_id) / SECTOR_SIZE - 1;
    } else {
        max_sectors = eeprom_calc_user_space(board->flash_id) / SECTOR_SIZE;
    }
    if (esectors > max_sectors) {
        printf("File Size too large to fit\n");
        return -1;
    }
    printf("EEPROM sectors to write: %d, max sectors in area: %d\n", esectors, max_sectors);
    sec_addr = start_address;
    printf("Erasing EEPROM sectors starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    for (sector = 0; sector <= esectors; sector++) {
        erase_sector(self, sec_addr);
        sec_addr = sec_addr + SECTOR_SIZE;
        printf("E");
        fflush(stdout);
    }
    printf("\n");
    return 0;
}

int remote_write_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    int bytesread, i, bindex;
    char part_name[32];
    struct stat file_stat;
    FILE *fp;

    if (stat(bitfile_name, &file_stat) != 0) {
        printf("Can't find file %s\n", bitfile_name);
        return -1;
    }

    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return -1;
    }
    if (print_bitfile_header(fp, (char*) &part_name) == -1) {
        fclose(fp);
        return -1;
    }
/*    if (strcmp(part_name, active_board.fpga_part_number) != 0) {
        printf("Error: wrong bitfile destination device: %s, should be %s\n", part_name, active_board.fpga_part_number);
        fclose(fp);
        return -1;
    }*/
    if (check_boot(self) == -1) {
        write_boot(self);
    } else {
        printf("Boot sector OK\n");
    }
    if (check_boot(self) == -1) {
        printf("Failed to write valid boot sector\n");
        fclose(fp);
        return -1;
    }

    if (start_programming(self, start_address, file_stat.st_size) == -1) {
        fclose(fp);
        return -1;
    }
    printf("Programming EEPROM sectors starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    i = start_address;
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            write_flash_address(self, i);
            write_page(self, &file_buffer[bindex]);
            i += PAGE_SIZE;
            bindex += PAGE_SIZE;
        }
        printf("W");
        fflush(stdout);
    }

    fclose(fp);
    printf("\nBoard configuration updated successfully\n");
    return 0;
}

int remote_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    int bytesread, fl_addr, bindex;
    char part_name[32];
    struct stat file_stat;
    FILE *fp;
    int i;

    if (stat(bitfile_name, &file_stat) != 0) {
        printf("Can't find file %s\n", bitfile_name);
        return -1;
    }

    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return -1;
    }
    if (print_bitfile_header(fp, (char*) &part_name) == -1) {
        fclose(fp);
        return -1;
    }
/*    if (strcmp(part_name, active_board.fpga_part_number) != 0) {
        printf("Error: wrong bitfile destination device: %s, should be %s\n", part_name, active_board.fpga_part_number);
        fclose(fp);
        return -1;
    }*/
    if (check_boot(self) == -1) {
        printf("Error: BootSector is invalid\n");
        fclose(fp);
        return -1;
    } else {
        printf("Boot sector OK\n");
    }

    printf("Verifying EEPROM sectors starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    fl_addr = start_address;
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            read_page(self, fl_addr, &page_buffer);
            for (i = 0; i < PAGE_SIZE; i++, bindex++) {
                if (file_buffer[bindex] != page_buffer[i]) {
                   printf("\nError at 0x%X expected: 0x%X but read: 0x%X\n", bindex, file_buffer[bindex], page_buffer[i]);
                   return -1;
                }
            }
            fl_addr += PAGE_SIZE;
        }
        printf("V");
        fflush(stdout);
    }

    fclose(fp);
    printf("\nBoard configuration verified successfully\n");
    return 0;
}

// global functions

void open_spi_access_remote(llio_t *self) {
};

void close_spi_access_remote(llio_t *self) {
};
