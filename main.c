
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>

#include "anyio.h"
#include "spi_eeprom.h"

static int device_flag;
static int addr_flag;
static int write_flag;
static int verify_flag;
static int fallback_flag;
static int program_flag;
static int readhmid_flag;
static int sserial_flag;
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
    {"program", required_argument, 0, 'p'},
    {"readhmid", no_argument, &readhmid_flag, 1},
    {"sserial", no_argument, &sserial_flag, 1},
    {"rpo", required_argument, 0, 'r'},
    {"wpo", required_argument, 0, 'o'},
    {"lbp16", required_argument, 0, 'l'},
    {"info", required_argument, 0, 'i'},
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, &verbose_flag, 1},
    {0, 0, 0, 0}
};

void print_short_usage() {
    printf("Configuration and diagnostic tool for mesanet PCI(E)/ETH/EPP/USB boards\n");
    printf("Try `mesaflash --help' for more information\n");
}

void print_usage() {
    printf("Syntax:\n");
    printf("    mesaflash --device device_name [options]\n");
    printf("    mesaflash --device device_name [options] [--write filename [--fallback]]\n");
    printf("    mesaflash --device device_name [options] [--verify filename [--fallback]]\n");
    printf("    mesaflash --device device_name [options] [--program filename]\n");
    printf("    mesaflash --device device_name [options] [--readhmid]\n");
    printf("    mesaflash --device device_name [options] [--sserial]\n");
    printf("    mesaflash --device device_name [options] [--rpo address]\n");
    printf("    mesaflash --device device_name [options] [--wpo address=value]\n");
    printf("    mesaflash --device device_name [options] [--lbp16 <command>]\n");
    printf("    mesaflash --info file_name\n");
    printf("    mesaflash --help\n");
    printf("Options:\n");
    printf("  --device      select active device name. If no command is given it will detect board with given name and print info about it.\n");
    printf("  --addr <device_address>\n");
    printf("      select <device address> for looking for <device_name> (network C mask for ETH boards, serial port for USB boards)\n");
    printf("  --verbose     print detailed information while running commands\n");
    printf("Commands:\n");
    printf("  --write       writes a standard bitfile 'filename' configuration to the userarea of the EEPROM (IMPORTANT! 'filename' must be VALID FPGA configuration file)\n");
    printf("  --verify      verifies the EEPROM configuration against the bitfile 'filename'\n");
    printf("  --fallback    use the fallback area of the EEPROM\n");
    printf("  --program     writes a standard bitfile 'filename' configuration to the FPGA (IMPORTANT! 'filename' must be VALID FPGA configuration file)\n");
    printf("  --readhmid    print hostmot2 configuration in PIN file format\n");
    printf("  --sserial     print full information about all sserial remote boards\n");
    printf("  --rpo         read hostmot2 variable directly at 'address'\n");
    printf("  --wpo         write hostmot2 variable directly at 'address' with 'value'\n");
    printf("  --lbp16       run <command> directly by lbp16 interface module\n");
    printf("    available commands:\n");
    printf("      send_packet=hex_data    send packet created from <hex_data> and print returned data\n");
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
    if (argc == 1) {
        print_short_usage();
        return 0;
    }
    if (process_cmd_line(argc, argv) == -1)
        return -1;

    if (info_flag == 1) {
        anyio_bitfile_print_info(bitfile_name);
    } else if (device_flag == 1) {
        board_t *board = NULL;

        access.verbose = verbose_flag;
        access.pci = 1;
        if (addr_flag == 1) {
            access.eth = 1;
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
        anyio_dev_set_active(board);
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
                u8 packet[512];
                u8 *pch = lbp16_send_packet_data;
                u32 *ptr = (u32 *) packet;
                int i, recv;

                for (i = 0; i < 512, i < strlen(lbp16_send_packet_data); i++, pch += 2) {
                    char s[3] = {*pch, *(pch + 1), 0};
                    packet[i] = strtol(s, NULL, 16) & 0xFF;
                }
                lbp16_send_packet(&packet, i/2);
                recv = lbp16_recv_packet(&packet, 512);
                for (i = 0; i < recv; i++)
                    printf("%02X", packet[i]);
                printf("\n");
            }
        } else if (write_flag == 1) {
            if (board->llio.program_flash != NULL) {
                u32 addr = board->flash_start_address;

                if (fallback_flag == 1)
                    addr = FALLBACK_ADDRESS;
                board->llio.program_flash(&(board->llio), bitfile_name, addr);
            } else {
                printf("Board %s doesn't support flash write.\n", board->llio.board_name);
            }
        } else if (verify_flag == 1) {
            if (board->llio.verify_flash != NULL) {
                u32 addr = board->flash_start_address;

                if (fallback_flag == 1)
                    addr = FALLBACK_ADDRESS;
                board->llio.verify_flash(&(board->llio), bitfile_name, addr);
            } else {
                printf("Board %s doesn't support flash verification.\n", board->llio.board_name);
            }
        } else if (program_flag == 1) {
            if (board->llio.program_fpga != NULL)
                board->llio.program_fpga(&(board->llio), bitfile_name);
            else
                printf("Board %s doesn't support FPGA programming.\n", board->llio.board_name);
        } else {
            anyio_dev_print_info(board);
        }
    }

    return 0;
}
