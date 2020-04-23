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

#include <sys/stat.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "types.h"
#include "common.h"
#include "eeprom.h"
#include "bitfile.h"
#include "lbp.h"
#include "hostmot2.h"
#include "usb_boards.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;
static u8 file_buffer[SECTOR_SIZE];

int usb_read(llio_t *self, u32 addr, void *buffer, int size) {
    while (size > 0) {
        lbp_read(addr & 0xFFFF, buffer);
        addr += 4;
        buffer += 4;
        size -= 4;
    }
    return 0;
}

int usb_write(llio_t *self, u32 addr, void *buffer, int size) {
    while (size > 0) {
        lbp_write(addr & 0xFFFF, buffer);
        addr += 4;
        buffer += 4;
        size -= 4;
    }
    return 0;
}

static int usb_program_fpga(llio_t *self, char *bitfile_name) {
    board_t *board = self->board;
    int bindex, bytesread;
    char part_name[32];
    struct stat file_stat;
    FILE *fp;
    u8 cmd = '0';

    if (stat(bitfile_name, &file_stat) != 0) {
        printf("Can't find file %s\n", bitfile_name);
        return -1;
    }
    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return -1;
    }
    if (print_bitfile_header(fp, (char*) &part_name, board->llio.verbose) == -1) {
        fclose(fp);
        return -1;
    }

    printf("Programming FPGA...\n");
    printf("  |");
    fflush(stdout);
    
    lbp_send(&cmd, 1);
    lbp_send(&cmd, 1);
    lbp_send(&cmd, 1);
    lbp_send(&cmd, 1);
    // program the FPGA
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            file_buffer[bindex] = bitfile_reverse_bits(file_buffer[bindex]);
            bindex++;
        }
        lbp_send(&file_buffer, bytesread);
        printf("W");
        fflush(stdout);
    }

    printf("\n");
    fclose(fp);

    return 0;
}

static int usb_board_open(board_t *board) {
    return 0;
}

static int usb_board_close(board_t *board) {
    return 0;
}

int usb_boards_init(board_access_t *access) {
    lbp_init(access);
    return 0;
}

void usb_boards_cleanup(board_access_t *access) {
    lbp_release();
}

void usb_boards_scan(board_access_t *access) {
    board_t *board = &boards[boards_count];
    u8 cmd, data;
    char dev_name[4];

    board_init_struct(board);

    data = lbp_read_ctrl(LBP_CMD_READ_COOKIE);
    if (data == LBP_COOKIE) {
        dev_name[0] = lbp_read_ctrl(LBP_CMD_READ_DEV_NAME0);
        dev_name[1] = lbp_read_ctrl(LBP_CMD_READ_DEV_NAME1);
        dev_name[2] = lbp_read_ctrl(LBP_CMD_READ_DEV_NAME2);
        dev_name[3] = lbp_read_ctrl(LBP_CMD_READ_DEV_NAME3);

        if (strncmp(dev_name, "7I64", 4) == 0) {
            board->type = BOARD_USB;
            strcpy(board->dev_addr, access->dev_addr);
            strncpy(board->llio.board_name, dev_name, 4);
            board->llio.verbose = access->verbose;

            boards_count++;
        } else if (strncmp(dev_name, "7I43", 4) == 0) { // found 7i43 with HOSTMOT2
            board->type = BOARD_USB;
            board->mode = BOARD_MODE_FPGA;
            strcpy(board->dev_addr, access->dev_addr);
            strcpy(board->llio.board_name, "7I43");
            board->llio.num_ioport_connectors = 2;
            board->llio.pins_per_connector = 24;
            board->llio.ioport_connector_name[0] = "P3";
            board->llio.ioport_connector_name[1] = "P4";
            board->llio.num_leds = 8;
            board->llio.read = &usb_read;
            board->llio.write = &usb_write;

            board->open = &usb_board_open;
            board->close = &usb_board_close;
            board->print_info = &usb_print_info;
            board->llio.verbose = access->verbose;

            u32 cookie;
            lbp_read(HM2_COOKIE_REG, &cookie);
            if (cookie == HM2_COOKIE) {
                u32 fpga_size;
                u32 addr;

                lbp_read(HM2_IDROM_ADDR, &addr);
                lbp_read((addr & 0xFFFF) + offsetof(hm2_idrom_desc_t, fpga_size), &fpga_size);
                if (fpga_size == 400) {
                    board->llio.fpga_part_number = "3s400tq144";
                } else {
                    board->llio.fpga_part_number = "3s200tq144";
                }
            }

            boards_count++;
        }
        return;
    }

    cmd = '1';
    lbp_send(&cmd, 1);
    lbp_recv(&data, 1);
    if ((data & 0x01) == 0) {  // found 7i43 without flashed FPGA
        board->type = BOARD_USB;
        board->mode = BOARD_MODE_CPLD;
        strcpy(board->dev_addr, access->dev_addr);
        strcpy(board->llio.board_name, "7I43");
        board->llio.num_ioport_connectors = 2;
        board->llio.pins_per_connector = 24;
        board->llio.ioport_connector_name[0] = "P3";
        board->llio.ioport_connector_name[1] = "P4";
        cmd = '0';
        lbp_send(&cmd, 1);
        lbp_recv(&data, 1);
        if (data & 0x01)
            board->llio.fpga_part_number = "3s400tq144";
        else
            board->llio.fpga_part_number = "3s200tq144";
        board->llio.num_leds = 8;
        board->llio.program_fpga = &usb_program_fpga;
        board->llio.verbose = access->verbose;
        board->open = &usb_board_open;
        board->close = &usb_board_close;
        board->print_info = &usb_print_info;

        boards_count++;
        return;
    }
}

void usb_boards_release(board_access_t *access) {
    lbp_release();
}

void usb_print_info(board_t *board) {
    printf("\nUSB device %s at %s\n", board->llio.board_name, board->dev_addr);
    if (board->llio.verbose == 1) {
        if (board->mode == BOARD_MODE_CPLD) {
            printf("  controlled by CPLD\n");
            lbp_print_info();
        } else if (board->mode == BOARD_MODE_FPGA) {
            printf("  controlled by FPGA\n");
        }
    }
}
