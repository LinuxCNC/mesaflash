
#ifndef __USB_BOARDS_H
#define __USB_BOARDS_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "hostmot2.h"

typedef struct {
    u8 flash_id;
    llio_t llio;
} usb_board_t;

void usb_boards_init();
void usb_boards_scan();

#endif
