
#ifndef __EPP_BOARDS_H
#define __EPP_BOARDS_H

#include "anyio.h"

#define EPP_STATUS_OFFSET   1
#define EPP_CONTROL_OFFSET  2
#define EPP_ADDRESS_OFFSET  3
#define EPP_DATA_OFFSET     4

#define ECP_CONFIG_A_HIGH_OFFSET    0
#define ECP_CONFIG_B_HIGH_OFFSET    1
#define ECP_CONTROL_HIGH_OFFSET     2

#define EPP_ADDR_AUTOINCREMENT  0x8000

int epp_boards_init(board_access_t *access);
void epp_boards_cleanup(board_access_t *access);
void epp_boards_scan(board_access_t *access);
void epp_print_info(board_t *board);

#endif
