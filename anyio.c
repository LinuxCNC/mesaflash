
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

board_t boards[MAX_BOARDS];
int boards_count;

int anyio_init(board_access_t *access) {
    int ret;

    if (access == NULL)
        return;
    if (access->pci == 1) {
        ret = pci_boards_init(access);
        if (ret != 0)
            return ret;
    }
    if (access->epp == 1) {
        ret = epp_boards_init(access);
        if (ret != 0)
            return ret;
    }
    if (access->usb == 1) {
        ret = usb_boards_init(access);
        if (ret != 0)
            return ret;
    }
    if (access->eth == 1) {
        ret = eth_boards_init(access);
        if (ret != 0)
            return ret;
    }
    if (access->spi == 1) {
        ret = spi_boards_init(access);
        if (ret != 0)
            return ret;
    }
    return 0;
}

void anyio_cleanup(board_access_t *access) {
    if (access == NULL)
        return;
    if (access->pci == 1)
        pci_boards_cleanup(access);
    if (access->epp == 1)
        epp_boards_cleanup(access);
    if (access->usb == 1)
        usb_boards_cleanup(access);
    if (access->eth == 1)
        eth_boards_cleanup(access);
    if (access->spi == 1)
        spi_boards_cleanup(access);
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
}

board_t *anyio_get_dev(board_access_t *access) {
    int i;
    board_t *board = NULL;

    if (access == NULL)
        return;
    for (i = 0; i < boards_count; i++) {
        if (strncmp(access->device_name, boards[i].llio.board_name, strlen(access->device_name)) == 0) {
            board = &boards[i];
            return board;
        }
    }
    return NULL;
}

void anyio_list_dev(board_access_t *access) {
    int i;

    if (access == NULL)
        return;
    for (i = 0; i < boards_count; i++) {
        board_t *board = &boards[i];

        anyio_dev_print_info(board);
    }
}

void anyio_open_dev(board_t *board) {
    if (board == NULL)
        return;

    if (board->open != NULL)
        board->open(board);
    else
        printf("ERROR: board %s doesn't support device opening\n", board->llio.board_name);
}

void anyio_close_dev(board_t *board) {
    if (board == NULL)
        return;

    if (board->close != NULL)
        board->close(board);
    else
        printf("ERROR: board %s doesn't support device closing\n", board->llio.board_name);
}

void anyio_dev_print_info(board_t *board) {
    if (board == NULL)
        return;
    switch (board->type) {
        case BOARD_ETH:
            eth_print_info(board);
            break;
        case BOARD_PCI:
            pci_print_info(board);
            break;
        case BOARD_EPP:
            epp_print_info(board);
            break;
        case BOARD_USB:
            usb_print_info(board);
            break;
        case BOARD_SPI:
            spi_print_info(board);
            break;
    }
}

void anyio_dev_print_hm2_info(board_t *board) {
    if (board == NULL)
        return;
    hm2_read_idrom(&(board->llio));
    hm2_print_pin_file(&(board->llio));
}

void anyio_dev_print_sserial_info(board_t *board) {
    if (board == NULL)
        return;
    hm2_read_idrom(&(board->llio));
    sserial_module_init(&(board->llio));
}

void anyio_bitfile_print_info(char *bitfile_name) {
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
