
#ifndef __ETH_BOARDS_H
#define __ETH_BOARDS_H

#include "anyio.h"

int eth_boards_init(board_access_t *access);
void eth_boards_cleanup(board_access_t *access);
void eth_boards_scan(board_access_t *access);
void eth_print_info(board_t *board);
inline int eth_socket_send_packet(void *packet, int size);
inline int eth_socket_recv_packet(void *buffer, int size);
void eth_socket_set_dest_ip(char *addr_name);

#endif
