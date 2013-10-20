
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>

#include "anyio.h"

static int list_flag;
static board_access_t access;

static struct option long_options[] = {
    {"list", no_argument, &list_flag, 1},
    {0, 0, 0, 0}
};

void print_usage(void) {
   printf("Configuration program for mesanet PCI(E)/ETH/LPT/USB boards\n");
   printf("    mesaflash --list\n");
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
    process_cmd_line(argc, argv);

    if (list_flag == 1) {
        access.net_addr = "192.168.1.0";
//        access.pci = 1;
//        access.eth = 1;
        access.lpt_base_addr = 0xDC00;
        access.lpt = 1;
//        access.usb = 1;
        boards_init(&access);
        boards_scan(&access);
    }
    
    return 0;
}
