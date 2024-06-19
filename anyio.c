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
#include "serial_boards.h"

supported_board_entry_t supported_boards[] = {
    {"ETHER", BOARD_ETH | BOARD_WILDCARD},
    {"SPI", BOARD_SPI | BOARD_WILDCARD},
    {"7I92", BOARD_ETH},
    {"7I92T", BOARD_ETH},
    {"7I93", BOARD_ETH},
    {"7I94", BOARD_ETH},
    {"7I94T", BOARD_ETH},
    {"7I95", BOARD_ETH},
    {"7I95T", BOARD_ETH},
    {"7I96", BOARD_ETH},
    {"7I96S", BOARD_ETH},
    {"7I97", BOARD_ETH},
    {"7I97T", BOARD_ETH},
    {"7I98", BOARD_ETH},
    {"7I80HD-16", BOARD_ETH},
    {"7I80HD-25", BOARD_ETH},
    {"7I80DB-16", BOARD_ETH},
    {"7I80DB-25", BOARD_ETH},
    {"7I80HDT", BOARD_ETH},
    {"7I76E", BOARD_ETH},
    {"7I76EU", BOARD_ETH},

    {"LITEHM2", BOARD_ETH},

    {"4I74", BOARD_PCI},
    {"5I24", BOARD_PCI},
    {"5I25", BOARD_PCI},
    {"6I24", BOARD_PCI},
    {"6I25", BOARD_PCI},
    {"5I20", BOARD_PCI},
    {"RECOVER", BOARD_PCI},
    {"4I65", BOARD_PCI},
    {"4I68", BOARD_PCI},
    {"5I21", BOARD_PCI},
    {"5I22", BOARD_PCI},
    {"5I23", BOARD_PCI},
    {"4I69", BOARD_PCI},
    {"3X20", BOARD_PCI},

    {"7C80", BOARD_MULTI_INTERFACE | BOARD_EPP | BOARD_SPI},
    {"7C81", BOARD_MULTI_INTERFACE | BOARD_EPP | BOARD_SPI},
    {"7I43", BOARD_MULTI_INTERFACE | BOARD_EPP | BOARD_USB},
    {"7I90HD", BOARD_MULTI_INTERFACE | BOARD_EPP | BOARD_SPI | BOARD_SER},
    {"7I64", BOARD_MULTI_INTERFACE | BOARD_USB | BOARD_SPI},

    {"AUTO", BOARD_MULTI_INTERFACE | BOARD_WILDCARD | BOARD_USB | BOARD_EPP | BOARD_SPI | BOARD_SER | BOARD_PCI},
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
#if MESAFLASH_IO
    if (access->open_iface & BOARD_PCI)
        pci_boards_cleanup(access);
    if (access->open_iface & BOARD_EPP)
        epp_boards_cleanup(access);
#endif
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
        anyio_print_supported_board_names();
        return -1;
    }

    access->open_iface = 0;
    if (supported_board->type & BOARD_WILDCARD)
        access->open_iface = BOARD_WILDCARD;
    if (access->type == BOARD_ANY) {
        if (supported_board->type & BOARD_MULTI_INTERFACE) {
            printf("ERROR: you must select transport layer for board\n");
            return -1;
        }
        if (supported_board->type & BOARD_ETH) {
            ret = eth_boards_init(access);
            access->open_iface |= BOARD_ETH;
            ret = eth_boards_scan(access);
            if (ret < 0) {
                return ret;
            }
        }
#if MESAFLASH_IO
        if (supported_board->type & BOARD_PCI) {
            ret = pci_boards_init(access);
            if (ret < 0) {
                return ret;
            }
            access->open_iface |= BOARD_PCI;
            pci_boards_scan(access);
        }
        if (supported_board->type & BOARD_EPP) {
            ret = epp_boards_init(access);
            if (ret < 0) {
                return ret;
            }
            access->open_iface |= BOARD_EPP;
            epp_boards_scan(access);
        }
#endif
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
        if (supported_board->type & BOARD_SER) {
            ret = serial_boards_init(access);
            access->open_iface |= BOARD_SER;
            serial_boards_scan(access);
        }
    } else {
        if (access->type & BOARD_ETH) {
            ret = eth_boards_init(access);
            access->open_iface |= BOARD_ETH;
            eth_boards_scan(access);
        }
#if MESAFLASH_IO
        if (access->type & BOARD_PCI) {
            ret = pci_boards_init(access);
            if (ret < 0) {
                return ret;
            }
            access->open_iface |= BOARD_PCI;
            pci_boards_scan(access);
        }
        if (access->type & BOARD_EPP) {
            ret = epp_boards_init(access);
            if (ret < 0) {
                return ret;
            }
            access->open_iface |= BOARD_EPP;
            epp_boards_scan(access);
        }
#endif
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
        if (access->type & BOARD_SER) {
            ret = serial_boards_init(access);
            access->open_iface |= BOARD_SER;
            serial_boards_scan(access);
        }
    }

    return 0;
}

board_t *anyio_get_dev(board_access_t *access, int board_number) {
    int i, j;

    if (access == NULL) {
        return NULL;
    }
    for (i = 0, j = 0; i < boards_count; i++) {
        board_t *board = NULL;
        board = &boards[i];
        if (((strncmp(access->device_name, board->llio.board_name, strlen(access->device_name)) == 0) &&
        (strlen(access->device_name) == strlen(board->llio.board_name))) || access->open_iface & BOARD_WILDCARD) {
            j++;
            if (j == board_number) {
                return board;
            }
        }
    }
    return NULL;
}

int anyio_dev_write_flash(board_t *board, char *bitfile_name, int fallback_flag, int fix_boot_flag, int sha256_check_flag) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.write_flash != NULL) {
        u32 addr = board->flash_start_address;

        if (fallback_flag == 1) {
            if (board->fpga_type == FPGA_TYPE_EFINIX) {
					addr = EFINIX_FALLBACK_ADDRESS;
            } else {
 					addr = XILINX_FALLBACK_ADDRESS;
            }
        }
        ret = board->llio.write_flash(&(board->llio), bitfile_name, addr, fix_boot_flag, sha256_check_flag);
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
            if (board->fpga_type == FPGA_TYPE_EFINIX) {
					addr = EFINIX_FALLBACK_ADDRESS;
            } else {
 					addr = XILINX_FALLBACK_ADDRESS;
            }
         }   
        ret = board->llio.verify_flash(&(board->llio), bitfile_name, addr);
    } else {
        printf("ERROR: Board %s doesn't support flash verification.\n", board->llio.board_name);
        return -EINVAL;
    }
    return ret;
}

int anyio_dev_backup_flash(board_t *board, char *bitfile_name) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.backup_flash != NULL) {
        ret = board->llio.backup_flash(&(board->llio), bitfile_name);
    } else {
        printf("ERROR: Board %s doesn't support backup flash.\n", board->llio.board_name);
        return -EINVAL;
    }
    return ret;
}

int anyio_dev_restore_flash(board_t *board, char *bitfile_name) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.restore_flash != NULL) {
        ret = board->llio.restore_flash(&(board->llio), bitfile_name);
    } else {
        printf("ERROR: Board %s doesn't support restore flash.\n", board->llio.board_name);
        return -EINVAL;
    }
    return ret;
}

int anyio_dev_program_fpga(board_t *board, char *bitfile_name) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.reset == NULL) {
        printf("ERROR: Board %s doesn't support FPGA resetting.\n", board->llio.board_name);
        return -EINVAL;
    }
    if (board->llio.program_fpga == NULL) {
        printf("ERROR: Board %s doesn't support FPGA programming.\n", board->llio.board_name);
        return -EINVAL;
    }

    ret = board->llio.reset(&(board->llio));
    if (ret != 0) {
       return ret;
    }
    board->llio.program_fpga(&(board->llio), bitfile_name);
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
int anyio_dev_set_led_mode(board_t *board, char *lbp16_set_led_mode) {
    int ret;

    if (board == NULL) {
        return -EINVAL;
    }
    if ((board->type & BOARD_ETH) == 0) {
        return -EPERM;
    }

    ret = eth_set_led_mode(lbp16_set_led_mode);
    if (ret == 0) {
        printf("Board LED mode updated successfully.\n");
        printf("You must power cycle board to load updated eeprom settings.\n");
    }
    return ret;
}

int anyio_dev_reload(board_t *board, int fallback_flag) {
    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.reload == NULL) {
        printf("ERROR: Board %s doesn't support FPGA configuration reloading.\n", board->llio.board_name);
        return -EINVAL;
    }
    return board->llio.reload(&(board->llio), fallback_flag);
}

int anyio_dev_reset(board_t *board) {
    if (board == NULL) {
        return -EINVAL;
    }
    if (board->llio.reset == NULL) {
        printf("ERROR: Board %s doesn't support FPGA resetting.\n", board->llio.board_name);
        return -EINVAL;
    }
    return board->llio.reset(&(board->llio));
}

void anyio_dev_print_hm2_info(board_t *board, int xml_flag) {
    if (board == NULL) {
        return;
    }
    hm2_read_idrom(&(board->llio.hm2));
    hm2_print_pin_file(&(board->llio), xml_flag);
}

void anyio_dev_print_pin_descriptors(board_t *board) {
    if (board == NULL) {
        return;
    }
    hm2_read_idrom(&(board->llio.hm2));
    hm2_print_pin_descriptors(&board->llio);
}

void anyio_dev_print_localio_descriptors(board_t *board) {
    if (board == NULL) {
        return;
    }
    hm2_read_idrom(&(board->llio.hm2));
    hm2_print_localio_descriptors(&board->llio);
}

void anyio_dev_print_sserial_info(board_t *board) {
    if (board == NULL) {
        return;
    }
    hm2_read_idrom(&(board->llio.hm2));
    sserial_module_init(&(board->llio));
}

void anyio_bitfile_print_info(char *bitfile_name, int verbose_flag) {
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
    print_bitfile_header(fp, (char*) &part_name, verbose_flag);
    fclose(fp);
}

void anyio_print_supported_board_names() {
    printf("Supported card names are:\n");
    for (size_t i=0; supported_boards[i].name != NULL; i++) {
        printf("%10s,",supported_boards[i].name);
        if ((((i+1) % 6) == 0) & (i != 0)) { printf("\n"); };
    }   
    printf("\n");     
}

void anyio_dev_enable_all_module_outputs(board_t *board) {
    if (board == NULL) {
        return;
    }
    hm2_read_idrom(&(board->llio.hm2));
    hm2_enable_all_module_outputs(&(board->llio.hm2));
}

void anyio_dev_safe_io(board_t *board) {
    if (board == NULL) {
        return;
    }
    hm2_read_idrom(&(board->llio.hm2));
    hm2_safe_io(&(board->llio.hm2));
}


