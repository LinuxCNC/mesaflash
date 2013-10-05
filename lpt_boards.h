
#ifndef __LPT_BOARDS_H
#define __LPT_BOARDS_H

#include <pci/pci.h>
#include <linux/parport.h>

#include "hostmot2.h"

#define MAX_LPT_BOARDS          8

#define LPT_EPP_STATUS_OFFSET   1
#define LPT_EPP_CONTROL_OFFSET  2
#define LPT_EPP_ADDRESS_OFFSET  3
#define LPT_EPP_DATA_OFFSET     4

#define LPT_ECP_CONFIG_A_HIGH_OFFSET    0
#define LPT_ECP_CONFIG_B_HIGH_OFFSET    1
#define LPT_ECP_CONTROL_HIGH_OFFSET     2

#define LPT_ADDR_AUTOINCREMENT  0x8000

typedef struct {
    unsigned short base;
    unsigned short base_hi;
    struct pardevice *linux_dev;
    void *region;
    void *region_hi;
    int dev_fd;
    int epp_wide;

    llio_t llio;
} lpt_board_t;

void lpt_boards_init();
void lpt_boards_scan();

#endif
