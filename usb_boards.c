
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
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
}

void usb_boards_scan(board_access_t *access) {
#ifdef __linux__
    u8 cmd, data;

    data = lbp_read_ctrl(LBP_CMD_READ_COOKIE);
    if (data != 0x5A) {
        printf("ERROR: no LBP remote device found on %s\n", access->dev_addr);
        return;
    }

#endif
}

void usb_print_info(board_t *board) {
    lbp_release();
}
