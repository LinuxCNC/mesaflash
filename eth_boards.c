
#include <pci/pci.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>

#include "eth_boards.h"
#include "lbp16.h"

eth_board_t eth_boards[MAX_ETH_BOARDS];
int boards_count;

int eth_read(llio_t *this, u32 addr, void *buffer, int size) {
    return lbp16_hm2_read(addr, buffer, size);
}

int eth_write(llio_t *this, u32 addr, void *buffer, int size) {
    return lbp16_hm2_write(addr, buffer, size);
}

void eth_init(eth_access_t *access) {
    lbp16_init();
}

void eth_scan(eth_access_t *access) {
    lbp16_cmd_addr packet, packet2;
    char addr[16];
    int i;
    int send = 0, recv = 0;
    u32 cookie;
    char *ptr;
    struct timespec tv, tvret;    // for nanosleep in linux, on windows there is only Sleep(ms)

    lbp16_socket_nonblocking();

    tv.tv_sec = 0;
    tv.tv_nsec = 5000000;

    strncpy(addr, access->net_addr, 16);
    ptr = strrchr(addr, '.');
    *ptr = '\0';

    LBP16_INIT_PACKET4(packet, CMD_READ_HM2_COOKIE, HM2_COOKIE_REG);
    for (i = 1; i < 256; i++) {
        char addr_name[32];

        sprintf(addr_name, "%s.%d", addr, i);
        lbp16_socket_set_dest_ip(addr_name);
        send = lbp16_send_packet(&packet, sizeof(packet));
        nanosleep(&tv, &tvret);
        recv = lbp16_recv_packet(&cookie, sizeof(cookie));

        if ((recv > 0) && (cookie == HM2_COOKIE)) {
            char buff[20];
            eth_board_t *board = &eth_boards[boards_count];

            lbp16_socket_blocking();
            LBP16_INIT_PACKET4(packet2, CMD_READ_BOARD_INFO_ADDR16_INCR(8), 0);
            memset(buff, 0, sizeof(buff));
            send = lbp16_send_packet(&packet2, sizeof(packet2));
            recv = lbp16_recv_packet(&buff, sizeof(buff));

            if (strncmp(buff, "7I80DB-16", 9) == 0) {
                strncpy(board->ip_addr, lbp16_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 4;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "J2";
                board->llio.ioport_connector_name[1] = "J3";
                board->llio.ioport_connector_name[2] = "J4";
                board->llio.ioport_connector_name[3] = "J5";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
            } else if (strncmp(buff, "7I80DB-25", 9) == 0) {
                strncpy(board->ip_addr, lbp16_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 4;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "J2";
                board->llio.ioport_connector_name[1] = "J3";
                board->llio.ioport_connector_name[2] = "J4";
                board->llio.ioport_connector_name[3] = "J5";
                board->llio.fpga_part_number = "6slx25ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
            } else if (strncmp(buff, "7I80HD-16", 9) == 0) {
                strncpy(board->ip_addr, lbp16_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.ioport_connector_name[2] = "P3";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
            } else if (strncmp(buff, "7I80HD-25", 9) == 0) {
                strncpy(board->ip_addr, lbp16_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.ioport_connector_name[2] = "P3";
                board->llio.fpga_part_number = "6slx25ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
            } else {
                printf("Unsupported ethernet device %s at %s\n", buff, lbp16_socket_get_src_ip());
                continue;
            }
            printf("\nETH device %s at ip=%s\n", buff, lbp16_socket_get_src_ip());
            board->llio.private = board;
            boards_count++;

            hm2_read_idrom(&(board->llio));
            lbp16_print_info();

            lbp16_socket_nonblocking();
        }
    }
}

void eth_release(eth_access_t *access) {
    lbp16_release();
}

