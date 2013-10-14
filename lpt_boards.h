
#ifndef __LPT_BOARDS_H
#define __LPT_BOARDS_H

#define LPT_EPP_STATUS_OFFSET   1
#define LPT_EPP_CONTROL_OFFSET  2
#define LPT_EPP_ADDRESS_OFFSET  3
#define LPT_EPP_DATA_OFFSET     4

#define LPT_ECP_CONFIG_A_HIGH_OFFSET    0
#define LPT_ECP_CONFIG_B_HIGH_OFFSET    1
#define LPT_ECP_CONTROL_HIGH_OFFSET     2

#define LPT_ADDR_AUTOINCREMENT  0x8000

void lpt_boards_init();
void lpt_boards_scan();
void lpt_print_info(board_t *board);

#endif
