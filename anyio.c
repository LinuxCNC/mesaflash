
#include <stdlib.h>

#include "anyio.h"
#include "eth_boards.h"
#include "pci_boards.h"
#include "lpt_boards.h"
#include "usb_boards.h"

board_t boards[MAX_BOARDS];
int boards_count;

void boards_init(board_access_t *access) {
    pci_boards_init();
    lpt_boards_init();
    usb_boards_init();
    eth_boards_init(access);
}

void boards_scan(board_access_t *access) {
    if (access == NULL)
        return;
    if (access->pci == 1)
        pci_boards_scan();
    if (access->lpt == 1)
        lpt_boards_scan();
    if (access->usb == 1)
        usb_boards_scan();
    if (access->eth == 1)
        eth_boards_scan(access);
}

void board_print_info(board_t *board) {
    if (board == NULL)
        return;
    switch (board->type) {
        case BOARD_ETH:
            eth_print_info(board);
            break;
        case BOARD_PCI:
            pci_print_info(board);
            break;
        case BOARD_LPT:
            lpt_print_info(board);
            break;
        case BOARD_USB:
            usb_print_info(board);
            break;
    }
}
