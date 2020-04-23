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

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include "anyio.h"
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

static int device_flag;
static int addr_flag;
static int addr_hi_flag;
static int write_flag;
static int fix_boot_flag;
static int verify_flag;
static int auto_verify_flag = 1;
static int fallback_flag;
static int recover_flag;
static int program_flag;
static int readhmid_flag;
static int print_pd_flag;
static int reload_flag;
static int reset_flag;
static int sserial_flag;
static int epp_flag;
static int usb_flag;
static int spi_flag;
static int serial_flag;
static int rpo_flag;
static int wpo_flag;
static u16 rpo_addr;
static u16 wpo_addr;
static u32 wpo_data;
static int set_flag;
static int xml_flag;
static char *lbp16_set_ip_addr;
static int info_flag;
static int verbose_flag;
static char bitfile_name[255];
static board_access_t access;

static struct option long_options[] = {
    {"device", required_argument, 0, 'd'},
    {"addr", required_argument, 0, 'a'},
    {"addr_hi", required_argument, 0, 'b'},
    {"write", required_argument, 0, 'w'},
    {"no-auto-verify", no_argument, &auto_verify_flag, 0},
    {"fix-boot-block", no_argument, &fix_boot_flag, 1},
    {"verify", required_argument, 0, 'v'},
    {"fallback", no_argument, &fallback_flag, 1},
    {"recover", no_argument, &recover_flag, 1},
    {"program", required_argument, 0, 'p'},
    {"readhmid", no_argument, &readhmid_flag, 1},
    {"print-pd", no_argument, &print_pd_flag, 1},
    {"reload", no_argument, &reload_flag, 1},
    {"reset", no_argument, &reset_flag, 1},
    {"sserial", no_argument, &sserial_flag, 1},
    {"epp", no_argument, &epp_flag, 1},
    {"usb", no_argument, &usb_flag, 1},
    {"spi", no_argument, &spi_flag, 1},
    {"serial", no_argument, &serial_flag, 1},
    {"rpo", required_argument, 0, 'r'},
    {"wpo", required_argument, 0, 'o'},
    {"set", required_argument, 0, 's'},
    {"xml", no_argument, &xml_flag, 1},
    {"info", required_argument, 0, 'i'},
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, &verbose_flag, 1},
    {0, 0, 0, 0}
};

void print_short_usage() {
    printf("Mesaflash version 3.3.0~pre (built on %s %s with libpci %s)\n", __DATE__, __TIME__, PCILIB_VERSION);
    printf("Configuration and diagnostic tool for Mesa Electronics PCI(E)/ETH/EPP/USB/SPI boards\n");
    printf("(C) 2013-2015 Michael Geszkiewicz (contact: micges@wp.pl)\n");
    printf("(C) Mesa Electronics\n");
    printf("Try 'mesaflash --help' for more information\n");
}

void print_usage() {
    printf("Syntax:\n");
    printf("  mesaflash --device device_name [options]\n");
    printf("  mesaflash --device device_name [options] --write filename\n");
    printf("  mesaflash --device device_name [options] --verify filename\n");
    printf("  mesaflash --device device_name [options] --program filename\n");
    printf("  mesaflash --device device_name [options] --readhmid\n");
    printf("  mesaflash --device device_name [options] --reload | --reset\n");
    printf("  mesaflash --device device_name [options] --sserial\n");
    printf("  mesaflash --device device_name [options] --rpo address\n");
    printf("  mesaflash --device device_name [options] --wpo address=value\n");
    printf("  mesaflash --device device_name [options] --set ip=n.n.n.n\n");
    printf("  mesaflash --info file_name\n");
    printf("  mesaflash --help\n");
    printf("\n");
    printf("Options:\n");
    printf("  --device <name>   Select active device name. If no command is given it\n");
    printf("                    will detect board with given name and print info\n");
    printf("                    about it.\n");
    printf("  --addr <dev>      Select <dev> for looking for <name> (IP address for\n");
    printf("                    Ethernet boards, serial device name for USB boards\n");
    printf("                    and serial boards, SPI device name for SPI boards)\n");
    printf("  --addr_hi         Set the high register address for the EPP interface.\n");
    printf("  --epp             Use EPP interface to connect to board, only for boards\n");
    printf("                    with multiple interfaces (7i43, 7i90, 7i64).\n");
    printf("  --usb             Use USB interface to connect to board, only for boards\n");
    printf("                    with multiple interfaces (7i43, 7i90, 7i64).\n");
    printf("  --spi             Use SPI interface to connect to board, only for boards\n");
    printf("                    with multiple interfaces (7i43, 7i90, 7i64).\n");
    printf("  --serial          Use serial interface to connect to board, only for\n");
    printf("                    boards with multiple interfaces (7i43, 7i90, 7i64).\n");
    printf("  --fallback        Use the fallback area of the EEPROM while executing\n");
    printf("                    commands.\n");
    printf("  --recover         Access board using PCI bridge GPIO (currently\n");
    printf("                    only 6I24/6I25).\n");
    printf("  --xml             Format output from 'readhmid' command into XML.\n");
    printf("  --verbose         Print detailed information while running commands.\n");
    printf("\n");
    printf("Commands:\n");
    printf("  --write           Writes a standard bitfile 'filename' configuration to\n");
    printf("                    the userarea of the EEPROM (IMPORTANT! 'filename' must\n");
    printf("                    be VALID FPGA configuration file).\n");
    printf("  --fix-boot-block  If a write operation does not detect a valid boot\n");
    printf("                    block, write one.\n");
    printf("  --no-auto-verify  Don't automatically verify after writing.\n");
    printf("  --verify          Verifies the EEPROM configuration against the\n");
    printf("                    bitfile 'filename'.\n");
    printf("  --program         Writes a standard bitfile 'filename' configuration to\n");
    printf("                    the FPGA (IMPORTANT! 'filename' must be VALID FPGA\n");
    printf("                    configuration file).\n");
    printf("  --readhmid        Print hostmot2 configuration in PIN file format.\n");
    printf("  --print-pd        Print hostmot2 Pin Descriptors.\n");
    printf("  --reload          Do full FPGA reload from flash (only Ethernet and\n");
    printf("                    pci boards).\n");
    printf("  --reset           Do full firmware reset (only Ethernet and serial boards).\n");
    printf("  --sserial         Print full information about all sserial remote boards.\n");
    printf("  --rpo             Read hostmot2 variable directly at 'address'.\n");
    printf("  --wpo             Write hostmot2 variable directly at 'address'\n");
    printf("                    with 'value'.\n");
    printf("  --set             Set board IP address in eeprom to n.n.n.n (only\n");
    printf("                    Ethernet boards).\n");
    printf("  --info            Print info about configuration in 'file_name'.\n");
    printf("  --help            Print this help message.\n");
}

int process_cmd_line(int argc, char *argv[]) {
    int c;

    while (1) {
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) break;
        switch (c) {
            case 0: {
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0) break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
            }
            break;

            case 'd': {
                int i;

                if (device_flag > 0) {
                    printf("Error: multiple --device options\n");
                    exit(-1);
                }
                access.device_name = optarg;
                for (i = 0; optarg[i] != '\0'; i++)
                    access.device_name[i] = toupper(access.device_name[i]);

                device_flag++;
            }
            break;

            case 'a': {
                if (addr_flag > 0) {
                    printf("Error: multiple --addr options\n");
                    exit(-1);
                }
                access.dev_addr = optarg;
                addr_flag++;
            }
            break;

	        case 'b': {
                if (addr_hi_flag > 0) {
                    printf("Error: multiple --addr_hi options\n");
                    exit(-1);
                }
                access.dev_hi_addr = optarg;
                addr_hi_flag++;
            }
	        break;

            case 'w': {
                if (write_flag > 0) {
                    printf("Error: multiple --write options\n");
                    exit(-1);
                }
                strncpy(bitfile_name, optarg, 255);
                write_flag++;
            }
            break;

            case 'p': {
                if (program_flag > 0) {
                    printf("Error: multiple --program options\n");
                    exit(-1);
                }
                strncpy(bitfile_name, optarg, 255);
                program_flag++;
            }
            break;

            case 'r': {
                if (rpo_flag > 0) {
                    printf("Error: multiple --rpo options\n");
                    exit(-1);
                }
                if (strncmp(optarg, "0x", 2) == 0) {
                    optarg[0] = '0';
                    optarg[1] = '0';
                    rpo_addr = strtol(optarg, NULL, 16);
                } else {
                    rpo_addr = strtol(optarg, NULL, 10);
                }
                rpo_flag++;
            }
            break;

            case 'o': {
                char *pch;

                if (wpo_flag > 0) {
                    printf("Error: multiple --wpo options\n");
                    exit(-1);
                }
                pch = strtok(optarg, "=");
                if (strncmp(pch, "0x", 2) == 0) {
                    pch[0] = '0';
                    pch[1] = '0';
                    wpo_addr = strtol(pch, NULL, 16);
                } else {
                    wpo_addr = strtol(pch, NULL, 10);
                }
                pch = strtok(NULL, "=");
                if (strncmp(pch, "0x", 2) == 0) {
                    pch[0] = '0';
                    pch[1] = '0';
                    wpo_data = strtol(pch, NULL, 16);
                } else {
                    wpo_data = strtol(pch, NULL, 10);
                }
                wpo_flag++;
            }
            break;

            case 's': {
                if (set_flag > 0) {
                    printf("Error: multiple --set options\n");
                    exit(-1);
                }
                if (strncmp(optarg, "ip=", 3) == 0) {
                    char *pch;

                    pch = strtok(optarg, "=");
                    pch = strtok(NULL, "=");
                    lbp16_set_ip_addr = pch;
                } else {
                    printf("Error: Unknown set command syntax, see --help for examples\n");
                    exit(-1);
                }
                set_flag++;
            }
            break;

            case 'v': {
                if (verify_flag > 0) {
                    printf("Error: multiple --verify options\n");
                    exit(-1);
                }
                strncpy(bitfile_name, optarg, 255);
                verify_flag++;
            }
            break;

            case 'i': {
                if (info_flag > 0) {
                    printf("Error: multiple --info options\n");
                    exit(-1);
                }
                strncpy(bitfile_name, optarg, 255);
                info_flag++;
            }
            break;

            case 'h': {
                print_usage();
                exit(0);
            }
            break;

            case '?':
                /* getopt_long already printed an error message. */
                return -1;
                break;

            default:
                abort();
        }
    }

    access.type = BOARD_ANY;
    if ((epp_flag == 1) && (usb_flag == 0) && (spi_flag == 0) && (serial_flag == 0)) {
        access.type = BOARD_EPP;
    } else if ((epp_flag == 0) && (usb_flag == 1) && (spi_flag == 0) && (serial_flag == 0)) {
        access.type = BOARD_USB;
    } else if ((epp_flag == 0) && (usb_flag == 0) && (spi_flag == 1) && (serial_flag == 0)) {
        access.type = BOARD_SPI;
    } else if ((epp_flag == 0) && (usb_flag == 0) && (spi_flag == 0) && (serial_flag == 1)) {
        access.type = BOARD_SER;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int ret = 0;

    if (argc == 1) {
        print_short_usage();
        return 0;
    }
    if (process_cmd_line(argc, argv) == -1)
        return -1;

    if (info_flag == 1) {
        anyio_bitfile_print_info(bitfile_name, 1);
    } else if (device_flag == 1) {
        board_t *board = NULL;

        access.verbose = verbose_flag;
        access.recover = recover_flag;
        access.address = addr_flag;

        if (anyio_init(&access) != 0)
            exit(1);
        ret = anyio_find_dev(&access);
        if (ret < 0) {
            printf("No %s board found\n", access.device_name);
            return -1;
        }
        board = anyio_get_dev(&access, 1);
        if (board == NULL) {
            printf("No %s board found\n", access.device_name);
            return -1;
        }

        board->open(board);

        if (readhmid_flag == 1) {
            anyio_dev_print_hm2_info(board, xml_flag);
        } else if (print_pd_flag == 1) {
            anyio_dev_print_pin_descriptors(board);
        } else if (sserial_flag == 1) {
            anyio_dev_print_sserial_info(board);
        } else if (rpo_flag == 1) {
            u32 data;

            board->llio.read(&(board->llio), rpo_addr, &data, sizeof(u32));
            printf("%08X\n", data);
        } else if (wpo_flag == 1) {
            board->llio.write(&(board->llio), wpo_addr, &wpo_data, sizeof(u32));
        } else if (set_flag == 1) {
            ret = anyio_dev_set_remote_ip(board, lbp16_set_ip_addr);
        } else if (write_flag == 1) {
            ret = anyio_dev_write_flash(board, bitfile_name, fallback_flag, fix_boot_flag);
            if (ret == 0) {
                if (auto_verify_flag) {
                    ret = anyio_dev_verify_flash(board, bitfile_name, fallback_flag);
                }
            }
            if (ret == 0) {
                if (reload_flag == 1) {
                    ret = anyio_dev_reload(board, fallback_flag);
                    if (ret == -1) {
                        printf("\nYou must power cycle the hardware to load a new firmware.\n");
                    }
                } else if (board->llio.reload) {
                    printf("\nYou must power cycle the hardware or use the --reload command to load a new firmware.\n");
                } else {
                    printf("\nYou must power cycle the hardware\n");
                }
            }
        } else if (verify_flag == 1) {
            ret = anyio_dev_verify_flash(board, bitfile_name, fallback_flag);
        } else if (program_flag == 1) {
            ret = anyio_dev_program_fpga(board, bitfile_name);
        } else if (reload_flag == 1) {
            anyio_dev_reload(board, fallback_flag);
        } else if (reset_flag == 1) {
            anyio_dev_reset(board);
        } else {
           board->print_info(board);
        }
        board->close(board);
        anyio_cleanup(&access);
    } else {
        printf("No action requested. Please specify at least --device or --info.\n");
    }
    return ret;
}
