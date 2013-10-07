
#ifndef __ETH_BOARDS_H
#define __ETH_BOARDS_H

#include "hostmot2.h"

#define MAX_ETH_BOARDS 8

typedef struct {
    char ip_addr[16];
    u8 flash_id;
    llio_t llio;
} eth_board_t;

typedef struct {
    char *net_addr;
} eth_access_t;

void eth_init(eth_access_t *access);
void eth_scan(eth_access_t *access);
void eth_release(eth_access_t *access);

#endif
