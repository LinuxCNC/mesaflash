//
//    Copyright (C) 2013-2014 Michael Geszkiewicz
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#ifndef __EPP_BOARDS_H
#define __EPP_BOARDS_H

#include "boards.h"

#define EPP_STATUS_OFFSET   1
#define EPP_CONTROL_OFFSET  2
#define EPP_ADDRESS_OFFSET  3
#define EPP_DATA_OFFSET     4

#define ECP_CONFIG_A_HIGH_OFFSET    0
#define ECP_CONFIG_B_HIGH_OFFSET    1
#define ECP_CONTROL_HIGH_OFFSET     2

#define EPP_ADDR_AUTOINCREMENT  0x8000

void epp_addr8(board_t *board, u8 addr);
u8 epp_read8(board_t *board);
void epp_write8(board_t *board, u8 data);
int epp_boards_init(board_access_t *access);
void epp_boards_cleanup(board_access_t *access);
void epp_boards_scan(board_access_t *access);
void epp_print_info(board_t *board);

#endif
