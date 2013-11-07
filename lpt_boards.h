
#ifndef __LPT_BOARDS_H
#define __LPT_BOARDS_H

#include "anyio.h"

#define LPT_EPP_STATUS_OFFSET   1
#define LPT_EPP_CONTROL_OFFSET  2
#define LPT_EPP_ADDRESS_OFFSET  3
#define LPT_EPP_DATA_OFFSET     4

#define LPT_ECP_CONFIG_A_HIGH_OFFSET    0
#define LPT_ECP_CONFIG_B_HIGH_OFFSET    1
#define LPT_ECP_CONTROL_HIGH_OFFSET     2

#define LPT_ADDR_AUTOINCREMENT  0x8000

int lpt_boards_init(board_access_t *access);
void lpt_boards_scan(board_access_t *access);
void lpt_print_info(board_t *board);

#endif
