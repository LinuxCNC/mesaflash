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

static int device_flag;
static int addr_flag;
static int write_flag;
static int verify_flag;
static int fallback_flag;
static int recover_flag;
static int program_flag;
static int readhmid_flag;
static int sserial_flag;
static int list_flag;
static int rpo_flag;
static int wpo_flag;
static u16 rpo_addr;
static u16 wpo_addr;
static u32 wpo_data;
static int lbp16_flag;
static int lbp16_send_packet_flag;
static char *lbp16_send_packet_data;
static int info_flag;
static int verbose_flag;
static char bitfile_name[255];
static board_access_t access;

static struct option long_options[] = {
    {"device", required_argument, 0, 'd'},
    {"addr", required_argument, 0, 'a'},
    {"write", required_argument, 0, 'w'},
    {"verify", required_argument, 0, 'v'},
    {"fallback", no_argument, &fallback_flag, 1},
    {"recover", no_argument, &recover_flag, 1},
    {"program", required_argument, 0, 'p'},
    {"readhmid", no_argument, &readhmid_flag, 1},
    {"sserial", no_argument, &sserial_flag, 1},
    {"list", no_argument, &list_flag, 1},
    {"rpo", required_argument, 0, 'r'},
    {"wpo", required_argument, 0, 'o'},
    {"lbp16", required_argument, 0, 'l'},
    {"info", required_argument, 0, 'i'},
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, &verbose_flag, 1},
    {0, 0, 0, 0}
};

void print_short_usage() {
    printf("Configuration and diagnostic tool for Mesa Electronics PCI(E)/ETH/EPP/USB boards\n");
    printf("Try `mesaflash --help' for more information\n");
}

void print_usage() {
    printf("Syntax:\n");
    printf("  mesaflash [options]\n");
    printf("  mesaflash [options] --write filename\n");
    printf("  mesaflash [options] --verify filename\n");
    printf("  mesaflash [options] --program filename\n");
    printf("  mesaflash [options] --readhmid\n");
    printf("  mesaflash [options] --sserial\n");
    printf("  mesaflash [options] --rpo address\n");
    printf("  mesaflash [options] --wpo address=value\n");
    printf("  mesaflash [options] --lbp16 <command>\n");
    printf("  mesaflash --list\n");
    printf("  mesaflash --info file_name\n");
    printf("  mesaflash --help\n");
    printf("\nOptions:\n");
    printf("  --device      select active device name. If no command is given it will detect board with given name and print info about it.\n");
    printf("  --addr <device_address>\n");
    printf("      select <device address> for looking for <device_name> (network C mask for ETH boards, serial port for USB boards)\n");
    printf("  --fallback    use the fallback area of the EEPROM\n");
    printf("  --recover     access board using PCI bridge GPIO (currently only 6I25)\n");
    printf("  --verbose     print detailed information while running commands\n");
    printf("\nCommands:\n");
    printf("  --write       writes a standard bitfile 'filename' configuration to the userarea of the EEPROM (IMPORTANT! 'filename' must be VALID FPGA configuration file)\n");
    printf("  --verify      verifies the EEPROM configuration against the bitfile 'filename'\n");
    printf("  --program     writes a standard bitfile 'filename' configuration to the FPGA (IMPORTANT! 'filename' must be VALID FPGA configuration file)\n");
    printf("  --readhmid    print hostmot2 configuration in PIN file format\n");
    printf("  --sserial     print full information about all sserial remote boards\n");
    printf("  --rpo         read hostmot2 variable directly at 'address'\n");
    printf("  --wpo         write hostmot2 variable directly at 'address' with 'value'\n");
    printf("  --lbp16       run <command> directly by lbp16 interface module\n");
    printf("    available commands:\n");
    printf("      send_packet=hex_data    send packet created from <hex_data> and print returned data\n");
    printf("  --list        show list of all detected boards\n");
    printf("  --info        print info about configuration in 'file_name'\n");
    printf("  --help        print this help message\n");
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
                    printf("Error: multiply --device option\n");
                    exit(-1);
                }
                access.device_name = optarg;
                for (i = 0; optarg[i] != '\0'; i++)
                    access.device_name[i] = toupper(access.device_name[i]);

                device_flag++;
            }
            break;

            case 'a': {
                int i;

                if (addr_flag > 0) {
                    printf("Error: multiply --addr option\n");
                    exit(-1);
                }
                access.dev_addr = optarg;
                addr_flag++;
            }
            break;

            case 'w': {
                if (write_flag > 0) {
                    printf("Error: multiply --write option\n");
                    exit(-1);
                }
                strncpy(bitfile_name, optarg, 255);
                write_flag++;
            }
            break;

            case 'p': {
                if (program_flag > 0) {
                    printf("Error: multiply --program option\n");
                    exit(-1);
                }
                strncpy(bitfile_name, optarg, 255);
                program_flag++;
            }
            break;

            case 'r': {
                if (rpo_flag > 0) {
                    printf("Error: multiply --rpo option\n");
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
                    printf("Error: multiply --wpo option\n");
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

            case 'l': {
                if (lbp16_flag > 0) {
                    printf("Error: multiply --lbp16 option\n");
                    exit(-1);
                }
                if (strncmp(optarg, "send_packet", 11) == 0) {
                    char *pch;

                    lbp16_send_packet_flag++;
                    pch = strtok(optarg, "=");
                    pch = strtok(NULL, "=");
                    if (strncmp(pch, "0x", 2) == 0)
                        pch += 2;
                    lbp16_send_packet_data = pch;
                } else {
                    printf("Error: unknown lbp16 command\n");
                    exit(-1);
                }
                lbp16_flag++;
            }
            break;

            case 'v': {
                if (verify_flag > 0) {
                    printf("Error: multiply --verify option\n");
                    exit(-1);
                }
                strncpy(bitfile_name, optarg, 255);
                verify_flag++;
            }
            break;

            case 'i': {
                if (info_flag > 0) {
                    printf("Error: multiply --info option\n");
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
        anyio_bitfile_print_info(bitfile_name);
    } else if (list_flag == 1) {
        access.verbose = verbose_flag;
        access.pci = 1;
        if (addr_flag == 1) {
            access.eth = 1;
//            access.usb = 1;
//            access.epp = 1;
        }

        if (anyio_init(&access) != 0)
            exit(1);
        anyio_scan(&access);
        anyio_list_dev(&access);
        anyio_cleanup(&access);
    } else if (device_flag == 1) {
        board_t *board = NULL;

        access.verbose = verbose_flag;
        access.recover = recover_flag;
        access.pci = 1;
        if (addr_flag == 1) {
            access.address = 1;
            access.eth = 1;
            access.epp = 1;
//            access.usb = 1;
        }

        if (anyio_init(&access) != 0)
            exit(1);
        anyio_scan(&access);
        board = anyio_get_dev(&access);
        if (board == NULL) {
            printf("No %s board found\n", access.device_name);
            return -1;
        }

        board->open(board);

        if (readhmid_flag == 1) {
            anyio_dev_print_hm2_info(board);
        } else if (sserial_flag == 1) {
            anyio_dev_print_sserial_info(board);
        } else if (rpo_flag == 1) {
            u32 data;

            board->llio.read(&(board->llio), rpo_addr, &data, sizeof(u32));
            printf("%08X\n", data);
        } else if (wpo_flag == 1) {
            board->llio.write(&(board->llio), wpo_addr, &wpo_data, sizeof(u32));
        } else if (lbp16_flag == 1) {
            if (lbp16_send_packet_flag == 1) {
                ret = anyio_dev_send_packet(board, lbp16_send_packet_data);
            }
        } else if (write_flag == 1) {
            ret = anyio_dev_write_flash(board, bitfile_name, fallback_flag);
        } else if (verify_flag == 1) {
            ret = anyio_dev_verify_flash(board, bitfile_name, fallback_flag);
        } else if (program_flag == 1) {
            ret = anyio_dev_program_fpga(board, bitfile_name);
        } else {
           board->print_info(board);
        }
        board->close(board);
        anyio_cleanup(&access);
    }

    return ret;
}
