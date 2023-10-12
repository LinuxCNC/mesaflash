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

#ifndef __ETH_BOARDS_H
#define __ETH_BOARDS_H

#include "boards.h"

int eth_boards_init(board_access_t *access);
void eth_boards_cleanup(board_access_t *access);
int eth_boards_scan(board_access_t *access);
void eth_print_info(board_t *board);
int eth_send_packet(void *packet, int size);
int eth_recv_packet(void *buffer, int size);
int lbp16_read(u16 cmd, u32 addr, void *buffer, int size);
int lbp16_write(u16 cmd, u32 addr, void *buffer, int size);
void eth_socket_set_dest_ip(char *addr_name);
int eth_set_remote_ip(char *ip_addr);
int eth_set_led_mode(char *led_mode);


#endif
