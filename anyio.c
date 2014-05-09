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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "anyio.h"
#include "eeprom.h"
#include "bitfile.h"
#include "eth_boards.h"
#include "pci_boards.h"
#include "epp_boards.h"
#include "usb_boards.h"
#include "spi_boards.h"

supported_board_entry_t supported_boards[] = {
    {"7I92", BOARD_ETH},
    {"7I80", BOARD_ETH},
    {"7I76E", BOARD_ETH},

    {"4I74", BOARD_PCI},
    {"5I24", BOARD_PCI},
    {"5I25", BOARD_PCI},
    {"6I25", BOARD_PCI},
    {"5I20", BOARD_PCI},
    {"4I65", BOARD_PCI},
    {"4I68", BOARD_PCI},
    {"5I21", BOARD_PCI},
    {"5I22", BOARD_PCI},
    {"5I23", BOARD_PCI},
    {"4I69", BOARD_PCI},
    {"3X20", BOARD_PCI},

    {"7I43", BOARD_MULTI_INTERFACE | BOARD_EPP | BOARD_USB},
    {"7I90", BOARD_MULTI_INTERFACE | BOARD_EPP | BOARD_SPI},
    {"7I64", BOARD_MULTI_INTERFACE | BOARD_USB | BOARD_SPI},

    {NULL, 0},
};

board_t boards[MAX_BOARDS];
int boards_count;

int anyio_init(board_access_t *access) {
    if (access == NULL) {
        return -EINVAL;
    }
    return 0;
}

void anyio_cleanup(board_access_t *access) {
    if (access == NULL) {
        return;
    }
    if (access->open_iface & BOARD_ETH)
        eth_boards_cleanup(access);
    if (access->open_iface & BOARD_PCI)
        pci_boards_cleanup(access);
    if (access->open_iface & BOARD_EPP)
        epp_boards_cleanup(access);
    if (access->open_iface & BOARD_USB)
        usb_boards_cleanup(access);
    if (access->open_iface & BOARD_SPI)
        spi_boards_cleanup(access);
    if (access->open_iface & BOARD_SER)
        serial_boards_cleanup(access);
    access->open_iface = 0;
}

int anyio_find_dev(board_access_t *access) {
    int i, ret = 0;
    supported_board_entry_t *supported_board = NULL;

    if (access == NULL) {
        return -1;
    }

    for (i = 0; supported_boards[i].name != NULL; i++) {
        if (strncmp(supported_boards[i].name, access->device_name, strlen(supported_boards[i].name)) == 0) {
            supported_board = &supported_boards[i];
            break;
        }
    }

    if (supported_board == NULL) {
        printf("ERROR: Unsupported device %s\n", access->device_name);
        return -1;
    }

    access->open_iface = 0;
    if (access->type == BOARD_ANY) {
        if (supported_board->type & BOARD_MULTI_INTERFACE) {
            printf("ERROR: you must select transport layer for board\n");
            return -1;
        }
        if (supported_board->type & BOARD_ETH) {
            ret = eth_boards_init(access);
            access->open_iface |= BOARD_ETH;
            eth_boards_scan(access);
        }
        if (supported_board->type & BOARD_PCI) {
            ret = pci_boards_init(access);
            access->open_iface |= BOARD_PCI;
            pci_boards_scan(access);
        }
        if (supported_board->type & BOARD_EPP) {
            ret = epp_boards_init(access);
            access->open_iface |= BOARD_EPP;
            epp_boards_scan(access);
        }
        if (supported_board->type & BOARD_USB) {
            ret = usb_boards_init(access);
            access->open_iface |= BOARD_USB;
            usb_boards_scan(access);
        }
        if (supported_board->type & BOARD_SPI) {
            ret = spi_boards_init(access);
            access->open_iface |= BOARD_SPI;
            spi_boards_scan(access);
        }
    } else {
        if (access->type & BOARD_ETH) {
            ret = eth_boards_init(access);
            access->open_iface |= BOARD_ETH;
            eth_boards_scan(access);
        }
        if (access->type & BOARD_PCI) {
            ret = pci_boards_init(access);
            access->open_iface |= BOARD_PCI;
            pci_boards_scan(access);
        }
        if (access->type & BOARD_EPP) {
            ret = epp_boards_init(access);
            access->open_iface |= BOARD_EPP;
            epp_boards_scan(access);
        }
        if (access->type & BOARD_USB) {
            ret = usb_boards_init(access);
            access->open_iface |= BOARD_USB;
            usb_boards_scan(access);
        }
        if (access->type & BOARD_SPI) {
            ret = spi_boards_init(access);
            access->open_iface |= BOARD_SPI;
            spi_boards_scan(access);
        }
    }
}

void anyio_scan(board_access_t *access) {
    if (access == NULL)
        return;
    if (access->pci == 1)
        pci_boards_scan(access);
    if (access->epp == 1)
        epp_boards_scan(access);
    if (access->usb == 1)
        usb_boards_scan(access);
    if (access->eth == 1)
        eth_boards_scan(access);
    if (access->spi == 1)
        spi_boards_scan(access);
    if (access->serial == 1)
        serial_boards_scan(access);
}

board_t *anyio_get_dev(board_access_t *access, int board_number) {
    int i, j;
    board_t *board = NULL;

    if (access == NULL) {
        return NULL;
    }
    for (i = 0, j = 0; i < boards_count; i++) {
        if (strncmp(access->device_name, boards[i].llio.board_name, strlen(access->device_name)) == 0) {
            j++;
            if (j == board_number) {
                board = &boards[i];
                return board;
            }
        }
    }
    return NULL;
}

int anyio_dev_write_flash(board_t *board, char *bitfile_name, int fallback_flag) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.write_flash != NULL) {
        u32 addr = board->flash_start_address;

        if (fallback_flag == 1) {
            addr = FALLBACK_ADDRESS;
        }
        ret = board->llio.write_flash(&(board->llio), bitfile_name, addr);
    } else {
        printf("ERROR: Board %s doesn't support flash writing.\n", board->llio.board_name);
        return -EINVAL;
    }
    return ret;
}

int anyio_dev_verify_flash(board_t *board, char *bitfile_name, int fallback_flag) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.verify_flash != NULL) {
        u32 addr = board->flash_start_address;

        if (fallback_flag == 1) {
            addr = FALLBACK_ADDRESS;
        }
        ret = board->llio.verify_flash(&(board->llio), bitfile_name, addr);
    } else {
        printf("ERROR: Board %s doesn't support flash verification.\n", board->llio.board_name);
        return -EINVAL;
    }
    return ret;
}

int anyio_dev_program_fpga(board_t *board, char *bitfile_name) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.reset != NULL) {
        ret = board->llio.reset(&(board->llio));
        if (ret != 0)
            return ret;
    } else {
        printf("ERROR: Board %s doesn't support FPGA resetting.\n", board->llio.board_name);
        return -EINVAL;
    }
    if (board->llio.program_fpga != NULL) {
        board->llio.program_fpga(&(board->llio), bitfile_name);
    } else {
        printf("ERROR: Board %s doesn't support FPGA programming.\n", board->llio.board_name);
        return -EINVAL;
    }
    return 0;
}

int anyio_dev_set_remote_ip(board_t *board, char *lbp16_set_ip_addr) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if ((board->type & BOARD_ETH) == 0) {
        return -EPERM;
    }

    ret = eth_set_remote_ip(lbp16_set_ip_addr);
    if (ret == 0) {
        printf("Board IP updated successfully.\n");
        printf("You must power cycle board to load updated eeprom settings.\n");
    }
    return ret;
}

int anyio_dev_reload(board_t *board, int fallback_flag) {
    if (board->type == BOARD_ETH) {
        return eth_board_reload(board, fallback_flag);
    } else if (board->type == BOARD_PCI) {
        return pci_board_reload(board, fallback_flag);
    } else {
        printf("ERROR: FPGA reload only supported by ethernet and pci cards.\n");
        return -1;
    }
}

int anyio_dev_reset(board_t *board) {
    if (board->type == BOARD_ETH) {
        return eth_board_reset(board);
    } else if (board->type == BOARD_PCI) {
        return board->llio.reset(&(board->llio));
    } else {
        printf("ERROR: FPGA reset only supported by ethernet cards.\n");
        return -1;
    }
}

void anyio_dev_print_hm2_info(board_t *board) {
    if (board == NULL) {
        return;
    }
    hm2_read_idrom(&(board->llio));
    hm2_print_pin_file(&(board->llio));
}

void anyio_dev_print_sserial_info(board_t *board) {
    if (board == NULL) {
        return;
    }
    hm2_read_idrom(&(board->llio));
    sserial_module_init(&(board->llio));
}

void anyio_bitfile_print_info(char *bitfile_name) {
    FILE *fp;
    char part_name[32];

    if (bitfile_name == NULL) {
        return;
    }
    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return;
    }
    print_bitfile_header(fp, (char*) &part_name);
    fclose(fp);
}
