#ifndef __ANYIO_H
#define __ANYIO_H

#include "hostmot2.h"

#define MAX_BOARDS 8

typedef enum {BOARD_ETH, BOARD_PCI, BOARD_LPT, BOARD_USB} board_type;

typedef struct {
    board_type type;
    u8 flash_id;

    char ip_addr[16];

    struct pci_dev *dev;
    void *base;
    int len;
    u16 ctrl_base_addr;
    u16 data_base_addr;

    unsigned short base_lo;
    unsigned short base_hi;
    void *region;
    void *region_hi;
    int epp_wide;

    llio_t llio;
} board_t;

typedef struct {
    char *device_name;
    int verbose;
    int pci;
    int lpt;
    int usb;
    int eth;
    char *net_addr;
    u16 lpt_base_addr;
    u16 lpt_base_hi_addr;
} board_access_t;

board_t boards[MAX_BOARDS];
int boards_count;

void boards_init(board_access_t *access);
void boards_scan(board_access_t *access);
board_t *boards_find(board_access_t *access);
void board_print_info(board_t *board);

#endif
