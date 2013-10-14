
#ifndef __USB_BOARDS_H
#define __USB_BOARDS_H

#include "anyio.h"

void usb_boards_init();
void usb_boards_scan();
void usb_print_info(board_t *board);

#endif
