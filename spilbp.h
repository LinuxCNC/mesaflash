
#ifndef __SPILBP_H
#define __SPILBP_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "common.h"

#define SPILBP_SENDRECV_DEBUG       0

#define SPILBP_MAX_PACKET_DATA_SIZE 0x7F

#define SPILBP_ADDR_AUTO_INC        0b0000100000000000
#define SPILBP_READ                 0b1010000000000000
#define SPILBP_WRITE                0b1011000000000000

typedef struct {
    u8 addr_hi;
    u8 addr_lo;
    u8 cmd_hi;
    u8 cmd_lo;
} spilbp_cmd_addr;

void spilbp_print_info();
void spilbp_init(board_access_t *access);
void spilbp_release();

#endif


