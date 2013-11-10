
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "anyio.h"
#include "usb_boards.h"
#include "common.h"
#include "spi_eeprom.h"
#include "bitfile.h"
#include "lbp.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;

int usb_boards_init(board_access_t *access) {
    lbp_init(access);
    return 0;
}

void usb_boards_scan(board_access_t *access) {
    board_t *board = &boards[boards_count];
    u8 cmd, data;
    u8 dev_name[4];

    data = lbp_read_ctrl(LBP_CMD_READ_COOKIE);
    if (data != LBP_COOKIE) {
        printf("ERROR: no LBP remote device found on %s\n", access->dev_addr);
        return;
    }
    dev_name[0] = lbp_read_ctrl(LBP_CMD_READ_DEV_NAME0);
    dev_name[1] = lbp_read_ctrl(LBP_CMD_READ_DEV_NAME1);
    dev_name[2] = lbp_read_ctrl(LBP_CMD_READ_DEV_NAME2);
    dev_name[3] = lbp_read_ctrl(LBP_CMD_READ_DEV_NAME3);

    if (strncmp(dev_name, "7I64", 4) == 0) {
        board->type = BOARD_USB;
        strcpy(board->dev_addr, access->dev_addr);
        strncpy(board->llio.board_name, dev_name, 4);
        board->llio.private = board;
        board->llio.verbose = access->verbose;

        boards_count++;
    }
}

void usb_boards_release(board_access_t *access) {
    lbp_release();
}

void usb_print_info(board_t *board) {
    printf("\nUSB device %s at %s\n", board->llio.board_name, board->dev_addr);
    if (board->llio.verbose == 1)
        lbp_print_info();
}
