
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "anyio.h"
#include "bitfile.h"
#include "eth_boards.h"
#include "pci_boards.h"
#include "lpt_boards.h"
#include "usb_boards.h"

board_t boards[MAX_BOARDS];
int boards_count;

void boards_init(board_access_t *access) {
    if (access == NULL)
        return;
    if (access->pci == 1)
        pci_boards_init(access);
    if (access->lpt == 1)
        lpt_boards_init(access);
    if (access->usb == 1)
        usb_boards_init(access);
    if (access->eth == 1)
        eth_boards_init(access);
}

void boards_scan(board_access_t *access) {
    if (access == NULL)
        return;
    if (access->pci == 1)
        pci_boards_scan(access);
    if (access->lpt == 1)
        lpt_boards_scan(access);
    if (access->usb == 1)
        usb_boards_scan(access);
    if (access->eth == 1)
        eth_boards_scan(access);
}

board_t *boards_find(board_access_t *access) {
    int i;
    board_t *board = NULL;

    if (access == NULL)
        return;
    for (i = 0; i < boards_count; i++) {
        if (strncmp(access->device_name, boards[i].llio.board_name, 4) == 0) {
            board = &boards[i];
            return board;
        }
    }
    return NULL;
}

void board_set_active(board_t *board) {
    if (board == NULL)
        return;
    switch (board->type) {
        case BOARD_ETH:
            lbp16_socket_set_dest_ip(board->ip_addr);
            break;
        case BOARD_PCI:
            break;
        case BOARD_LPT:
            break;
        case BOARD_USB:
            break;
    }
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

void board_print_hm2_info(board_t *board) {
    if (board == NULL)
        return;
    hm2_read_idrom(&(board->llio));
    hm2_print_pin_file(&(board->llio));
}

void board_print_sserial_info(board_t *board) {
    if (board == NULL)
        return;
    hm2_read_idrom(&(board->llio));
    sserial_module_init(&(board->llio));
}

void bitfile_print_info(char *bitfile_name) {
    FILE *fp;
    char part_name[32];

    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return;
    }
    print_bitfile_header(fp, (char*) &part_name);
    fclose(fp);
}
