
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
    if (data == LBP_COOKIE) {
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
	} else {
		cmd = '1';
        lbp_send(&cmd, 1);
        lbp_recv(&data, 1);
        if ((data & 0x01) == 0) {  // found 7i43
			board->type = BOARD_USB;
			strcpy(board->dev_addr, access->dev_addr);
			strncpy(board->llio.board_name, "7I43", 4);
            board->llio.num_ioport_connectors = 2;
            board->llio.pins_per_connector = 24;
            board->llio.ioport_connector_name[0] = "P3";
            board->llio.ioport_connector_name[1] = "P4";
            cmd = '0';
			lbp_send(&cmd, 1);
            lbp_recv(&data, 1);
            if (data & 0x01)
				board->llio.fpga_part_number = "3s400tq144";
			else 
				board->llio.fpga_part_number = "3s200tq144";
            board->llio.num_leds = 8;
			board->llio.private = board;
			board->llio.verbose = access->verbose;

			boards_count++;
        }
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
