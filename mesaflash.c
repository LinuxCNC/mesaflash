//
//    Copyright (C) 2020 Sebastian Kuzminsky <seb@highlab.com>
//    Copyright (C) 2013-2015 Michael Geszkiewicz
//    Copyright (C) Mesa Electronics
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

#ifndef VERSION
#define VERSION "3.5.11"
#endif

static int device_flag;
static int addr_flag;
static int addr_hi_flag;
static int write_flag;
static int fix_boot_flag;
static int verify_flag;
static int backup_flash_flag;
static int restore_flash_flag;
static int sha256_check_flag;
static int auto_verify_flag = 1;
static int fallback_flag;
static int recover_flag;
static int program_flag;
static int readhmid_flag;
static int print_pd_flag;
static int print_lio_flag;
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
static int ipaddr_flag;
static int ledmode_flag;
static int enable_all_mod_flag;
static int safe_io_flag;
static int xml_flag;
static char *lbp16_set_ip_addr;
static char *lbp16_set_led_mode;
static int info_flag;
static int verbose_flag;
static char bitfile_name[255];
static board_access_t access;
static int bob_hints[6];

#define array_size(x)  ((sizeof(x) / sizeof(x[0])))

static struct option long_options[] = {
    {"device", required_argument, 0, 'd'},
    {"addr", required_argument, 0, 'a'},
    {"addr_hi", required_argument, 0, 'b'},
    {"write", required_argument, 0, 'w'},
    {"no-auto-verify", no_argument, &auto_verify_flag, 0},
    {"fix-boot-block", no_argument, &fix_boot_flag, 1},
    {"verify", required_argument, 0, 'v'},
    {"backup-flash", required_argument, 0, 'f'},
    {"restore-flash", required_argument, 0, 'n'},
    {"sha256-check", no_argument, &sha256_check_flag, 1},
    {"fallback", no_argument, &fallback_flag, 1},
    {"recover", no_argument, &recover_flag, 1},
    {"program", required_argument, 0, 'p'},
    {"readhmid", no_argument, &readhmid_flag, 1},
    {"print-pd", no_argument, &print_pd_flag, 1},
    {"print-lio", no_argument, &print_lio_flag, 1},
    {"reload", no_argument, &reload_flag, 1},
    {"reset", no_argument, &reset_flag, 1},
    {"sserial", no_argument, &sserial_flag, 1},
    {"epp", no_argument, &epp_flag, 1},
    {"usb", no_argument, &usb_flag, 1},
    {"spi", no_argument, &spi_flag, 1},
    {"serial", no_argument, &serial_flag, 1},
    {"rpo", required_argument, 0, 'r'},
    {"wpo", required_argument, 0, 'o'},
    {"enable-all-mod", no_argument, &enable_all_mod_flag, 1},
    {"safe-io", no_argument, &safe_io_flag, 1},
    {"set", required_argument, 0, 's'},
    {"xml", no_argument, &xml_flag, 1},
    {"dbname1", required_argument, NULL, '1'},
    {"dbname2", required_argument, NULL, '2'},
    {"dbname3", required_argument, NULL, '3'},
    {"dbname4", required_argument, NULL, '4'},
    {"dbname5", required_argument, NULL, '5'},
    {"dbname6", required_argument, NULL, '6'},
    {"info", required_argument, 0, 'i'},
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, &verbose_flag, 1},
    {"version", no_argument, 0, 'x'},
    {0, 0, 0, 0}
};

void print_short_usage() {
    printf("Mesaflash version %s\n", VERSION);
    printf("Configuration and diagnostic tool for Mesa Electronics PCI(E)/ETH/EPP/USB/SPI boards\n");
    printf("Try 'mesaflash --help' for more information\n");
}

void print_version() {
    printf("Mesaflash version %s\n", VERSION);
}

void print_usage() {
    printf("Syntax:\n");
    printf("  mesaflash --device device_name [options]\n");
    printf("  mesaflash --device device_name [options] --write filename\n");
    printf("  mesaflash --device device_name [options] --verify filename\n");
    printf("  mesaflash --device device_name [options] --program filename\n");
    printf("  mesaflash --device device_name [options] --backup-flash filename | dirname\n");
    printf("  mesaflash --device device_name [options] --restore-flash filename\n");
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
    printf("                    with multiple interfaces (7c80, 7c81, 7i43, 7i90, 7i64).\n");
    printf("  --usb             Use USB interface to connect to board, only for boards\n");
    printf("                    with multiple interfaces (7c80, 7c81, 7i43, 7i90, 7i64).\n");
    printf("  --spi             Use SPI interface to connect to board, only for boards\n");
    printf("                    with multiple interfaces (7c80, 7c81, 7i43, 7i90, 7i64).\n");
    printf("  --serial          Use serial interface to connect to board, only for\n");
    printf("                    boards with multiple interfaces (7i43, 7i90, 7i64).\n");
    printf("  --fallback        Use the fallback area of the FLASH memory while executing\n");
    printf("                    commands.\n");
    printf("  --recover         Access board using PCI bridge GPIO (currently\n");
    printf("                    only 6I24/6I25). Use --recover --device recover --write.\n");
    printf("  --xml             Format output from 'readhmid' command into XML.\n");
    printf("  --dbname# <name>  Set daughter board name to <name> for FPGA connector <N> \n");
    printf("                    Allows readhmid to include daughterboard terminal names,\n");
    printf("                    where # can be in the range 1 to 6\n");
    printf("                    (1 means first FPGA connector).\n");
    printf("  --verbose         Print detailed information while running commands.\n");
    printf("\n");
    printf("Commands:\n");
    printf("  --write           Writes a standard bitfile 'filename' configuration to\n");
    printf("                    the userarea of the FLASH (IMPORTANT! 'filename' must\n");
    printf("                    be VALID FPGA configuration file).\n");
    printf("  --fix-boot-block  If a write operation does not detect a valid boot\n");
    printf("                    block, write one.\n");
    printf("  --no-auto-verify  Don't automatically verify after writing.\n");
    printf("  --verify          Verifies the FLASH configuration against the\n");
    printf("                    bitfile 'filename'.\n");
    printf("  --program         Writes a standard bitfile 'filename' configuration to\n");
    printf("                    the FPGA (IMPORTANT! 'filename' must be VALID FPGA\n");
    printf("                    configuration file).\n");
    printf("  --backup-flash    Backup all content the FLASH memory to the file 'filename'\n");
    printf("                    or to the directory 'dirname' with auto naming dump file.\n");
    printf("  --restore-flash   Restore all content the FLASH memory from a file 'filename'\n");
    printf("                    (IMPORTANT! Can't use a dump file from different types\n");
    printf("                    of boards. Unacceptable interrupt the restoring process.\n");
    printf("                    If the restoring process was interrupted, do not turn off\n");
    printf("                    the board power and do not reload board, and run restore\n");
    printf("                    process again).\n");
    printf("                    Required SHA256 checksum file 'filename.sha256'.\n");
    printf("  --sha256-check    Integrity check FPGA configuration bitfile before writing.\n");
    printf("                    Required SHA256 checksum file 'filename.sha256'.\n");
    printf("  --readhmid        Print hostmot2 configuration in PIN file format.\n");
    printf("  --print-pd        Print hostmot2 Pin Descriptors.\n");
    printf("  --print-lio       Print hostmot2 local I/O pins.\n");
    printf("  --reload          Do full FPGA reload from flash (only Ethernet, SPI and\n");
    printf("                    PCI boards).\n");
    printf("  --reset           Do full firmware reset (only Ethernet and serial boards).\n");
    printf("  --sserial         Print full information about all sserial remote boards.\n");
    printf("  --rpo addrs       Read hostmot2 register directly at 'addrs'.\n");
    printf("  --wpo addrs=data  Write hostmot2 register directly at 'addrs'\n");
    printf("                    with 'data'.\n");
    printf("  --set ip=n.n.n.n  Set board IP address in EEPROM memory to n.n.n.n (Only\n");
    printf("                    Ethernet boards).\n");
    printf("  --set ledmode=n   Set LED mode in EEPROM memory to n, 0 = Hostmot2, 1=Debug\n");
    printf("                    Default debug is RX packet count. (Only Ethernet boards).\n");
    printf("  --enable-all-mod  Enable all module outputs. For low level debugging\n");
    printf("                    Note that this is NOT safe for cards connected to equipment\n");
    printf("  --safe-io         Return all I/O to default power up state\n");
    printf("  --info            Print info about configuration in 'filename'.\n");
    printf("  --version         Print the version.\n");
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
                size_t len = strlen(optarg);
                if (len+1 > sizeof(bitfile_name)) {
                    printf("--write argument too long (max %zu)\n", sizeof(bitfile_name)-1);
                    return 1;
                }
                strcpy(bitfile_name, optarg);
                write_flag++;
            }
            break;

            case 'p': {
                if (program_flag > 0) {
                    printf("Error: multiple --program options\n");
                    exit(-1);
                }
                size_t len = strlen(optarg);
                if (len+1 > sizeof(bitfile_name)) {
                    printf("--program argument too long (max %zu)\n", sizeof(bitfile_name)-1);
                    return 1;
                }
                strcpy(bitfile_name, optarg);
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
                    ipaddr_flag = 1;
                } else if (strncmp(optarg, "ledmode=", 8) == 0) {
                    char *pch;

                    pch = strtok(optarg, "=");
                    pch = strtok(NULL, "=");
                    lbp16_set_led_mode = pch;
                    ledmode_flag = 1;
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
                size_t len = strlen(optarg);
                if (len+1 > sizeof(bitfile_name)) {
                    printf("--verify argument too long (max %zu)\n", sizeof(bitfile_name)-1);
                    return 1;
                }
                strcpy(bitfile_name, optarg);
                verify_flag++;
            }
            break;

            case 'f': {
                if (backup_flash_flag > 0) {
                    printf("Error: multiple --backup-flash options\n");
                    exit(-1);
                }
                size_t len = strlen(optarg);
                if (len+1 > sizeof(bitfile_name)) {
                    printf("--backup-flash argument too long (max %zu)\n", sizeof(bitfile_name)-1);
                    return 1;
                }
                strncpy(bitfile_name, optarg, 255);
                backup_flash_flag++;
            }
            break;

            case 'n': {
                if (restore_flash_flag > 0) {
                    printf("Error: multiple --restore-flash options\n");
                    exit(-1);
                }
                size_t len = strlen(optarg);
                if (len+1 > sizeof(bitfile_name)) {
                    printf("--restore-flash argument too long (max %zu)\n", sizeof(bitfile_name)-1);
                    return 1;
                }
                strncpy(bitfile_name, optarg, 255);
                restore_flash_flag++;
            }
            break;

            case 'i': {
                if (info_flag > 0) {
                    printf("Error: multiple --info options\n");
                    exit(-1);
                }
                size_t len = strlen(optarg);
                if (len+1 > sizeof(bitfile_name)) {
                    printf("--info argument too long (max %zu)\n", sizeof(bitfile_name)-1);
                    return 1;
                }
                strcpy(bitfile_name, optarg);
                info_flag++;
            }
            break;
 
            case 'h': {
                print_usage();
                exit(0);
            }
            break;

            case 'x': {
                print_version();
                exit(0);
            }
            break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            {
                int bob_idx = c - '1';
                int hint = hm2_find_bob_hint_by_name(optarg);
                if (hint == 0) {
                     printf("--dbname%c %s not recognized\n", c, optarg);
                     hm2_print_bob_hint_names();
                     exit(-1);
                }
                bob_hints[bob_idx] = hint;
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
        for(size_t i=0; i<array_size(bob_hints); i++) {
            if(bob_hints[i]) {
                board->llio.bob_hint[i] = bob_hints[i];
            }
        }

        if (readhmid_flag == 1) {
            anyio_dev_print_hm2_info(board, xml_flag);
        } else if (print_pd_flag == 1) {
            anyio_dev_print_pin_descriptors(board);
        } else if (print_lio_flag == 1) {
            anyio_dev_print_localio_descriptors(board);
        } else if (sserial_flag == 1) {
            anyio_dev_print_sserial_info(board);
        } else if (rpo_flag == 1) {
            u32 data;

            board->llio.read(&(board->llio), rpo_addr, &data, sizeof(u32));
            printf("%08X\n", data);
        } else if (wpo_flag == 1) {
            board->llio.write(&(board->llio), wpo_addr, &wpo_data, sizeof(u32));
        } else if (ipaddr_flag == 1) {
                ret = anyio_dev_set_remote_ip(board, lbp16_set_ip_addr);
        } else if (ledmode_flag == 1) {
                ret = anyio_dev_set_led_mode(board, lbp16_set_led_mode);                
        } else if (enable_all_mod_flag == 1) {
                anyio_dev_enable_all_module_outputs(board);                
        } else if (safe_io_flag == 1) {
                anyio_dev_safe_io(board);                
        } else if (write_flag == 1) {
            ret = anyio_dev_write_flash(board, bitfile_name, fallback_flag, fix_boot_flag, sha256_check_flag);
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
        } else if (backup_flash_flag == 1) {
            ret = anyio_dev_backup_flash(board, bitfile_name);
        } else if (restore_flash_flag == 1) {
            ret = anyio_dev_restore_flash(board, bitfile_name);
            if (ret == 0) {
                if (auto_verify_flag) {
                    ret = anyio_dev_verify_flash(board, bitfile_name, fallback_flag);
                }
            }
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
