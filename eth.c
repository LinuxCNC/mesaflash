
#include <pci/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>

#include "eth.h"
#include "lbp16.h"

eth_board_t eth_boards[MAX_ETH_BOARDS];
int boards_count;

int sd;
socklen_t len;
struct sockaddr_in server_addr, client_addr;

void eth_init(eth_access_t *access) {
}

void eth_scan(eth_access_t *access) {
    lbp16_cmd_addr packet, packet2;
    char addr[16];
    int i;
    int send = 0, recv = 0;
    u32 cookie;
    char *ptr;
    int val;
    struct timespec tv, tvret;    // for nanosleep in linux, on windows there is only Sleep(ms)

// open socket
    sd = socket (PF_INET, SOCK_DGRAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LBP16_UDP_PORT);
    len = sizeof(client_addr);

// set socket non blocking and setup sleep time
    val = fcntl(sd, F_GETFL);
    val = val | O_NONBLOCK;
    fcntl(sd, F_SETFL, val);

    tv.tv_sec = 0;
    tv.tv_nsec = 5000000;

    strncpy(addr, access->net_addr, 16);
    ptr = strrchr(addr, '.');
    *ptr = '\0';

    LBP16_INIT_PACKET4(packet, CMD_READ_HM2_COOKIE, HM2_COOKIE_REG);
    for (i = 1; i < 256; i++) {
        char addr_name[32];

        sprintf(addr_name, "%s.%d", addr, i);
        server_addr.sin_addr.s_addr = inet_addr(addr_name);
        send = sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        nanosleep(&tv, &tvret);
        recv = recvfrom(sd, (char*) &cookie, sizeof(cookie), 0, (struct sockaddr *) &client_addr, &len);

        if ((recv > 0) && (cookie == HM2_COOKIE)) {
            char buff[20];
            eth_board_t *board = &eth_boards[boards_count];

// set socket blocking
            val = fcntl(sd, F_GETFL);
            val = val & ~O_NONBLOCK;
            fcntl(sd, F_SETFL, val);
            LBP16_INIT_PACKET4(packet2, CMD_READ_BOARD_INFO_ADDR16_INCR(8), 0);
            memset(buff, 0, sizeof(buff));
            sendto(sd, (char*) &packet2, sizeof(packet2), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
            recvfrom(sd, (char*) &buff, sizeof(buff), 0, (struct sockaddr *) &client_addr, &len);

            if (strncmp(buff, "7I80DB-16", 9) == 0) {
                strncpy(board->ip_addr, inet_ntoa(client_addr.sin_addr), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 4;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "J2";
                board->llio.ioport_connector_name[1] = "J3";
                board->llio.ioport_connector_name[2] = "J4";
                board->llio.ioport_connector_name[3] = "J5";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
            } else if (strncmp(buff, "7I80DB-25", 9) == 0) {
                strncpy(board->ip_addr, inet_ntoa(client_addr.sin_addr), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 4;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "J2";
                board->llio.ioport_connector_name[1] = "J3";
                board->llio.ioport_connector_name[2] = "J4";
                board->llio.ioport_connector_name[3] = "J5";
                board->llio.fpga_part_number = "6slx25ftg256";
                board->llio.num_leds = 4;
            } else if (strncmp(buff, "7I80HD-16", 9) == 0) {
                strncpy(board->ip_addr, inet_ntoa(client_addr.sin_addr), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.ioport_connector_name[2] = "P3";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
            } else if (strncmp(buff, "7I80HD-25", 9) == 0) {
                strncpy(board->ip_addr, inet_ntoa(client_addr.sin_addr), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.ioport_connector_name[2] = "P3";
                board->llio.fpga_part_number = "6slx25ftg256";
                board->llio.num_leds = 4;
            } else {
                printf("Unsupported ethernet device %s at %s\n", buff, inet_ntoa(client_addr.sin_addr));
                goto error;
            }
            printf("board name: %s at ip=%s\n", buff, inet_ntoa(client_addr.sin_addr));
            board->llio.private = board;
            boards_count++;

// set socket non-blocking
            val = fcntl(sd, F_GETFL);
            val = val | O_NONBLOCK;
            fcntl(sd, F_SETFL, val);
        }
    }
    
error:
    close(sd);
}

int eth_read(llio_t *this, u32 addr, void *buffer, int size) {
    return lbp16_hm2_read(addr, buffer, size);
}

int eth_write(llio_t *this, u32 addr, void *buffer, int size) {
    return lbp16_hm2_write(addr, buffer, size);
}
