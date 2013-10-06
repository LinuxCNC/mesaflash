
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>

#include "eth_boards.h"
#include "pci_boards.h"
#include "lpt_boards.h"

static int list_flag;

static struct option long_options[] = {
    {"list", no_argument, &list_flag, 1},
    {0, 0, 0, 0}
};

void print_usage(void) {
   printf("Configuration program for mesanet PCI/ETH boards\n");
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
    eth_access_t eth_access;

    process_cmd_line(argc, argv);

    if (list_flag == 1) {
        eth_access.net_addr = "192.168.1.0";
        eth_init(&eth_access);
        eth_scan(&eth_access);
        pci_boards_init();
        pci_boards_scan();
        lpt_boards_init();
        lpt_boards_scan();
    }
    
    return 0;
}
