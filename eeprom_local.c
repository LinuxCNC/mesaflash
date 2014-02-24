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
#include <sys/io.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "eeprom.h"
#include "eeprom_local.h"
#include "hostmot2.h"
#include "bitfile.h"

extern u8 boot_block[BOOT_BLOCK_SIZE];
extern u8 page_buffer[PAGE_SIZE];
extern u8 file_buffer[SECTOR_SIZE];

spi_eeprom_dev_t access;

// spi access via hm2 registers

static void wait_for_data_hm2(llio_t *self) {
    u32 i = 0;
    u32 data = 0;

    for (i = 0; (((data & 0xFF) & HM2_DAV_MASK) == 0) && (i < 5000) ; i++) {
        self->read(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
    }
    if (i == 5000) {
        printf("%x timeout waiting for SPI data\n", data);
    }
}

static void set_cs_high_hm2(llio_t *self) {
    u32 data = 1;

    self->write(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
}

static void set_cs_low_hm2(llio_t *self) {
    u32 data = 0;

    self->write(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
}

static void prefix_hm2(llio_t *self) {
    set_cs_low_hm2(self);
}

static void suffix_hm2(llio_t *self) {
    set_cs_high_hm2(self);
}

static void send_byte_hm2(llio_t *self, u8 byte) {
    u32 data = byte;

    self->write(self, HM2_SPI_DATA_REG, &data, sizeof(data));
    wait_for_data_hm2(self);
}

static u8 recv_byte_hm2(llio_t *self) {
    u32 data = 0;
    u32 recv = 0;
    
    self->write(self, HM2_SPI_DATA_REG, &data, sizeof(data));
    wait_for_data_hm2(self);
    self->read(self, HM2_SPI_DATA_REG, &recv, sizeof(recv));
    return (u8) recv & 0xFF;
}

// spi access via gpio pins of xio2001 bridge on 6i25

static u16 GPIO_reg_val;

static void set_cs_high_gpio(llio_t *self) {
    board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val | 0x2;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_cs_low_gpio(llio_t *self) {
    board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val & (~ 0x2);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_din_high_gpio(llio_t *self) {
    board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val | 0x8;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_din_low_gpio(llio_t *self) {
    board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val & (~0x8);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_clock_high_gpio(llio_t *self) {
    board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val | 0x10;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_clock_low_gpio(llio_t *self) {
    board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val & (~ 0x10);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static int get_bit_gpio(llio_t *self) {
    board_t *board = self->private;
    u16 data;

    data = pci_read_word(board->dev, XIO2001_GPIO_DATA_REG);
    if (data & 0x4)
        return 1;
    else
        return 0;
}

static void prefix_gpio(llio_t *self) {
    set_cs_high_gpio(self);
    set_din_low_gpio(self);
    set_clock_low_gpio(self);
    set_cs_low_gpio(self);
}

static void suffix_gpio(llio_t *self) {
    set_cs_high_gpio(self);
    set_din_low_gpio(self);
}

static void init_gpio(llio_t *self) {
    board_t *board = self->private;

    pci_write_word(board->dev, XIO2001_GPIO_ADDR_REG, 0x001B);
    pci_write_word(board->dev, XIO2001_SBAD_STAT_REG, 0x0000);
    GPIO_reg_val = 0x0003;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void restore_gpio(llio_t *self) {
    board_t *board = self->private;

    GPIO_reg_val = 0x0003;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
    pci_write_word(board->dev, XIO2001_GPIO_ADDR_REG, 0x0000);
}

static void send_byte_gpio(llio_t *self, u8 byte) {
    u32 mask = DATA_MASK;
    int i;

    for (i = 0; i < CMD_LEN; i++) {
        if ((mask & byte) == 0)
            set_din_low_gpio(self);
        else
            set_din_high_gpio(self);
        mask >>= 1;
        set_clock_high_gpio(self);
        set_clock_low_gpio(self);
    }
}

static u8 recv_byte_gpio(llio_t *self) {
    u32 mask, data = 0;
    int i;

    mask = DATA_MASK;
    for (i = 0; i < DATA_LEN; i++) {
        if (get_bit_gpio(self) == 1)
            data |= mask;
        mask >>= 1;
        set_clock_high_gpio(self);
        set_clock_low_gpio(self);
    }
  return data;
}

// spi access via io ports like on 3x20

static void wait_for_data_io(llio_t *self) {
    board_t *board = self->private;
    u32 i = 0;
    u8 data = 0;

    for (i = 0; (((data & 0xFF) & IO_DAV_MASK) == 0) && (i < 5000) ; i++) {
        data = inb(board->data_base_addr + SPI_CS_OFFSET);
    }
    if (i == 5000) {
        printf("%x timeout waiting for SPI data\n", data);
    }
}

static void set_cs_high_io(llio_t *self) {
    board_t *board = self->private;

    outb(1, board->data_base_addr + SPI_CS_OFFSET);
}

static void set_cs_low_io(llio_t *self) {
    board_t *board = self->private;

    outb(0, board->data_base_addr + SPI_CS_OFFSET);
}

static void prefix_io(llio_t *self) {
    set_cs_low_io(self);
}

static void suffix_io(llio_t *self) {
    set_cs_high_io(self);
}

static void send_byte_io(llio_t *self, u8 byte) {
    board_t *board = self->private;

    outb(byte, board->data_base_addr + SPI_SREG_OFFSET);
}

static u8 recv_byte_io(llio_t *self) {
    board_t *board = self->private;

    outb(0, board->data_base_addr + SPI_SREG_OFFSET);
    wait_for_data_io(self);
    return inb(board->data_base_addr + SPI_SREG_OFFSET);
}

// pci flash

static void send_address(llio_t *self, u32 addr) {
    access.send_byte(self, (addr >> 16) & 0xFF);
    access.send_byte(self, (addr >> 8) & 0xFF);
    access.send_byte(self, addr & 0xFF);
}

static void write_enable(llio_t *self) {
    access.prefix(self);
    access.send_byte(self, SPI_CMD_WRITE_ENABLE);
    access.suffix(self);
}

static u8 read_status(llio_t *self) {
    u8 ret;

    access.prefix(self);
    access.send_byte(self, SPI_CMD_READ_STATUS);
    ret = access.recv_byte(self);
    access.suffix(self);
    return ret;
}

u8 read_flash_id(llio_t *self) {
    u8 ret;

    access.prefix(self);
    access.send_byte(self, SPI_CMD_READ_IDROM);
    send_address(self, 0); // three dummy bytes
    ret = access.recv_byte(self);
    access.suffix(self);
    return ret;
}

static void wait_for_write(llio_t *self) {
    u8 stat = read_status(self);

    while ((stat & WRITE_IN_PROGRESS_MASK) != 0) {
         stat = read_status(self);
    }
}

static u8 read_byte(llio_t *self, u32 addr) {
    u8 ret;

    access.prefix(self);
    access.send_byte(self, SPI_CMD_READ);
    send_address(self, addr);
    ret = access.recv_byte(self);
    access.suffix(self);
    return ret;
}

static void erase_sector(llio_t *self, u32 addr) {
    write_enable(self);
    access.prefix(self);
    access.send_byte(self, SPI_CMD_SECTOR_ERASE);
    send_address(self, addr);
    access.suffix(self);
    wait_for_write(self);
}

static void write_page(llio_t *self, u32 addr, char *buff, int buff_i) {
    int i;

    write_enable(self);
    access.prefix(self);
    access.send_byte(self, SPI_CMD_PAGE_WRITE);
    send_address(self, addr);// { note that add 0..7 should be 0}
    for (i = 0; i < PAGE_SIZE; i++) {
        access.send_byte(self, buff[buff_i + i]);
    }
    access.suffix(self);
    wait_for_write(self);
}

static int check_boot(llio_t *self) {
    board_t *board = self->private;
    int i;
    u8 data;

// if board doesn't support fallback there is no boot block
    if (board->fallback_support == 0)
        return 0;

    for (i = 0; i < BOOT_BLOCK_SIZE; i++) {
        data = read_byte(self, i);
        if (data != boot_block[i]) {
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
    write_page(self, 0, (char *) &file_buffer, 0);
    printf("BootBlock installed\n");
}

static int start_programming(llio_t *self, u32 start_address, int fsize) {
    u32 sec_addr;
    int esectors, sector, max_sectors;
    board_t *board = self->private;

    access.set_cs_high(self);

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
    access.set_cs_high(self);
    return 0;
}

static void done_programming(llio_t *self) {
    access.set_cs_high(self);
}

// global functions

int local_write_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    int bytesread, i;
    u32 eeprom_addr;
    char part_name[32];
    struct stat file_stat;
    u8 file_buffer[SECTOR_SIZE];
    FILE *fp;

    memset(&file_buffer, 0, sizeof(file_buffer));
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
    printf("Programming EEPROM area starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    eeprom_addr = start_address;
    while (!feof(fp)) {
      bytesread = fread(&file_buffer, 1, 8192, fp);
      i = 0;
      while (i < bytesread) {
        write_page(self, eeprom_addr, (char *) &file_buffer, i);
        i = i + PAGE_SIZE;
        eeprom_addr = eeprom_addr + PAGE_SIZE;
      }
      printf("W");
      fflush(stdout);
    }

    fclose(fp);
    printf("\nBoard configuration updated successfully\n");
    done_programming(self);
    return 0;
}

int local_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    int bytesread, i, bindex;
    u8 rdata, mdata;
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
        printf("Error: BootSector is invalid\n");
        fclose(fp);
        return -1;
    } else {
        printf("Boot sector OK\n");
    }

    printf("Verifying EEPROM sectors starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    i = start_address;
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            rdata = read_byte(self, bindex + i);
            mdata = file_buffer[bindex];
            if (mdata != rdata) {
                printf("\nError at 0x%X expected: 0x%X but read: 0x%X\n", i+bindex, mdata, rdata);
                return -1;
            }
            bindex = bindex + 1;
        }
        i = i + 8192;
        printf("V");
        fflush(stdout);
    }

    fclose(fp);
    printf("\nBoard configuration verified successfully\n");
    return 0;
}

void open_spi_access_local(llio_t *self) {
    board_t *board = self->private;

    switch (board->flash) {
        case BOARD_FLASH_NONE:
            break;
        case BOARD_FLASH_HM2:
            access.set_cs_low = &set_cs_low_hm2; 
            access.set_cs_high = &set_cs_high_hm2;
            access.prefix = &prefix_hm2;
            access.suffix = &suffix_hm2;
            access.send_byte = &send_byte_hm2;
            access.recv_byte = &recv_byte_hm2;
            break;
        case BOARD_FLASH_IO:
            access.set_cs_low = &set_cs_low_io; 
            access.set_cs_high = &set_cs_high_io;
            access.prefix = &prefix_io;
            access.suffix = &suffix_io;
            access.send_byte = &send_byte_io;
            access.recv_byte = &recv_byte_io;
            break;
        case BOARD_FLASH_GPIO:
            access.set_cs_low = &set_cs_low_gpio; 
            access.set_cs_high = &set_cs_high_gpio;
            access.prefix = &prefix_gpio;
            access.suffix = &suffix_gpio;
            access.send_byte = &send_byte_gpio;
            access.recv_byte = &recv_byte_gpio;
            init_gpio(self);
            break;
    }
};

void close_spi_access_local(llio_t *self) {
    board_t *board = self->private;

    switch (board->flash) {
        case BOARD_FLASH_NONE:
            break;
        case BOARD_FLASH_HM2:
            break;
        case BOARD_FLASH_IO:
            break;
        case BOARD_FLASH_GPIO:
            restore_gpio(self);
            break;
    }
}
