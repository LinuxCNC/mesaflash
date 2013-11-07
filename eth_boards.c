
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>

#include "common.h"
#include "bitfile.h"
#include "anyio.h"
#include "eth_boards.h"
#include "lbp16.h"
#include "spi_eeprom.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;
static u8 page_buffer[PAGE_SIZE];
extern u8 boot_block[BOOT_BLOCK_SIZE];
static u8 file_buffer[SECTOR_SIZE];

int eth_read(llio_t *this, u32 addr, void *buffer, int size) {
    return lbp16_hm2_read(addr, buffer, size);
}

int eth_write(llio_t *this, u32 addr, void *buffer, int size) {
    return lbp16_hm2_write(addr, buffer, size);
}

static void page_read(u32 addr, void *buff) {
    lbp16_cmd_addr_data32 write_addr_pck;
    lbp16_cmd_addr read_page_pck;
    int send, recv;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    LBP16_INIT_PACKET4(read_page_pck, CMD_READ_FPGA_FLASH_ADDR32(64), FLASH_DATA_REG);

    send = lbp16_send_packet(&write_addr_pck, sizeof(write_addr_pck));
    send = lbp16_send_packet(&read_page_pck, sizeof(read_page_pck));
    recv = lbp16_recv_packet(buff, PAGE_SIZE);
}

static void sector_erase(u32 addr) {
    lbp16_erase_flash_sector_packets sector_erase_pck;
    lbp16_cmd_addr_data32 write_addr_pck;
    int send, recv;
    u32 ignored;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    send = lbp16_send_packet(&write_addr_pck, sizeof(write_addr_pck));
    if (send < 0)
        printf("ERROR: %s(): %s\n", __func__, strerror(errno));

    LBP16_INIT_PACKET6(sector_erase_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A03);
    LBP16_INIT_PACKET8(sector_erase_pck.fl_erase_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_SEC_ERASE_REG, 0);
    send = lbp16_send_packet(&sector_erase_pck, sizeof(sector_erase_pck));
    if (send < 0)
        printf("ERROR: %s(): %s\n", __func__, strerror(errno));
    // packet read for board syncing
    recv = lbp16_read(CMD_READ_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, &ignored, 4);
}

static void page_write(void *buff) {
    lbp16_write_flash_page_packets write_page_pck;
    int send, recv;
    u32 ignored;

    LBP16_INIT_PACKET6(write_page_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A03);
    LBP16_INIT_PACKET4(write_page_pck.fl_write_page_pck, CMD_WRITE_FPGA_FLASH_ADDR32(64), FLASH_DATA_REG);
    memcpy(&write_page_pck.fl_write_page_pck.page, buff, 256);
    send = lbp16_send_packet(&write_page_pck, sizeof(write_page_pck));
    // packet read for board syncing
    recv = lbp16_read(CMD_READ_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, &ignored, 4);
}

static int check_boot() {
    int i;

    page_read(0x0, &page_buffer);
    for (i = 0; i < BOOT_BLOCK_SIZE; i++) {
        if (boot_block[i] != page_buffer[i]) {
            return -1;
        }
    }
    return 0;
}

static void write_boot() {
    printf("Erasing sector 0 for boot block\n");
    sector_erase(BOOT_ADDRESS);
    memset(&file_buffer, 0, PAGE_SIZE);
    memcpy(&file_buffer, &boot_block, BOOT_BLOCK_SIZE);
    page_write(file_buffer);
    printf("BootBlock installed\n");
}

static void write_flash_address(u32 addr) {
    lbp16_cmd_addr_data32 write_addr_pck;
    int send;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    send = lbp16_send_packet(&write_addr_pck, sizeof(write_addr_pck));
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
        sector_erase(sec_addr);
        sec_addr = sec_addr + SECTOR_SIZE;
        printf("E");
        fflush(stdout);
    }
    printf("\n");
    return 0;
}

int eth_program_flash(llio_t *self, char *bitfile_name, u32 start_address) {
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
    if (check_boot() == -1) {
        write_boot();
    } else {
        printf("Boot sector OK\n");
    }
    if (check_boot() == -1) {
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
            write_flash_address(i);
            page_write(&file_buffer[bindex]);
            i += PAGE_SIZE;
            bindex += PAGE_SIZE;
        }
        printf("W");
        fflush(stdout);
    }

    fclose(fp);
    printf("\nBoard configuration verified successfully\n");
    return 0;
}

int eth_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
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
    if (check_boot() == -1) {
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
            page_read(fl_addr, &page_buffer);
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

int eth_boards_init(board_access_t *access) {
    lbp16_init();
    return 0;
}

void eth_boards_scan(board_access_t *access) {
    lbp16_cmd_addr packet, packet2;
    char addr[16];
    int i;
    int send = 0, recv = 0;
    u32 cookie;
    char *ptr;

    lbp16_socket_nonblocking();

    strncpy(addr, access->net_addr, 16);
    ptr = strrchr(addr, '.');
    *ptr = '\0';

    LBP16_INIT_PACKET4(packet, CMD_READ_HM2_COOKIE, HM2_COOKIE_REG);
    for (i = 1; i < 256; i++) {
        char addr_name[32];

        sprintf(addr_name, "%s.%d", addr, i);
        lbp16_socket_set_dest_ip(addr_name);
        send = lbp16_send_packet(&packet, sizeof(packet));
        sleep_ns(2*1000*1000);
        recv = lbp16_recv_packet(&cookie, sizeof(cookie));

        if ((recv > 0) && (cookie == HM2_COOKIE)) {
            char buff[20];
            board_t *board = &boards[boards_count];

            lbp16_socket_blocking();
            LBP16_INIT_PACKET4(packet2, CMD_READ_BOARD_INFO_ADDR16_INCR(8), 0);
            memset(buff, 0, sizeof(buff));
            send = lbp16_send_packet(&packet2, sizeof(packet2));
            recv = lbp16_recv_packet(&buff, sizeof(buff));

            if (strncmp(buff, "7I80DB-16", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->ip_addr, lbp16_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 4;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "J2";
                board->llio.ioport_connector_name[1] = "J3";
                board->llio.ioport_connector_name[2] = "J4";
                board->llio.ioport_connector_name[3] = "J5";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else if (strncmp(buff, "7I80DB-25", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->ip_addr, lbp16_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 4;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "J2";
                board->llio.ioport_connector_name[1] = "J3";
                board->llio.ioport_connector_name[2] = "J4";
                board->llio.ioport_connector_name[3] = "J5";
                board->llio.fpga_part_number = "6slx25ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else if (strncmp(buff, "7I80HD-16", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->ip_addr, lbp16_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.ioport_connector_name[2] = "P3";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else if (strncmp(buff, "7I80HD-25", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->ip_addr, lbp16_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.ioport_connector_name[2] = "P3";
                board->llio.fpga_part_number = "6slx25ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else {
                printf("Unsupported ethernet device %s at %s\n", buff, lbp16_socket_get_src_ip());
                continue;
            }
            boards_count++;

            lbp16_socket_nonblocking();
        }
   }
   lbp16_socket_blocking();
}

void eth_boards_release(board_access_t *access) {
    lbp16_release();
}

void eth_print_info(board_t *board) {
    printf("\nETH device %s at ip=%s\n", board->llio.board_name, lbp16_socket_get_src_ip());
    if (board->llio.verbose == 1)
        lbp16_print_info();
}
