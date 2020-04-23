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

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/udp.h>      // struct udphdr
#elif _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "common.h"
#include "bitfile.h"
#include "eth_boards.h"
#include "eeprom_remote.h"
#include "lbp16.h"
#include "eeprom.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;

#ifdef __linux__
int sd;
socklen_t len;
#elif _WIN32
SOCKET sd;
int len;
#endif
struct sockaddr_in dst_addr, src_addr;

// eth access functions

inline int eth_send_packet(void *packet, int size) {
    return sendto(sd, (char*) packet, size, 0, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
}

inline int eth_recv_packet(void *buffer, int size) {
    return recvfrom(sd, (char*) buffer, size, 0, (struct sockaddr *) &src_addr, &len);
}

static void eth_socket_nonblocking() {
#ifdef __linux__
    int val = fcntl(sd, F_GETFL);

    val = val | O_NONBLOCK;
    fcntl(sd, F_SETFL, val);
#elif _WIN32
    u_long iMode = 1;

    ioctlsocket(sd, FIONBIO, &iMode);
#endif
}

static void eth_socket_blocking() {
#ifdef __linux__
    int val = fcntl(sd, F_GETFL);

    val = val & ~O_NONBLOCK;
    fcntl(sd, F_SETFL, val);
#elif _WIN32
    u_long iMode = 0;

    ioctlsocket(sd, FIONBIO, &iMode);
#endif
}

void eth_socket_set_dest_ip(char *addr_name) {
    dst_addr.sin_addr.s_addr = inet_addr(addr_name);
}

static char *eth_socket_get_src_ip() {
    return inet_ntoa(src_addr.sin_addr);
}

static int eth_read(llio_t *self, u32 addr, void *buffer, int size) {
    if ((size/4) > LBP16_MAX_PACKET_DATA_SIZE) {
        printf("ERROR: LBP16: Requested %d units to read, but protocol supports up to %d units to be read per packet\n", size/4, LBP16_MAX_PACKET_DATA_SIZE);
        return -1;
    }

    return lbp16_read(CMD_READ_HOSTMOT2_ADDR32_INCR(size/4), addr, buffer, size);
}

static int eth_write(llio_t *self, u32 addr, void *buffer, int size) {
    if ((size/4) > LBP16_MAX_PACKET_DATA_SIZE) {
        printf("ERROR: LBP16: Requested %d units to write, but protocol supports up to %d units to be write per packet\n", size/4, LBP16_MAX_PACKET_DATA_SIZE);
        return -1;
    }

    return lbp16_write(CMD_WRITE_HOSTMOT2_ADDR32_INCR(size/4), addr, buffer, size);
}

static int eth_board_open(board_t *board) {
    eth_socket_set_dest_ip(board->dev_addr);
    eeprom_init(&(board->llio));
    lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
    if (board->fallback_support == 1) {
        eeprom_prepare_boot_block(board->flash_id);
        board->flash_start_address = eeprom_calc_user_space(board->flash_id);
    } else {
        board->flash_start_address = 0;
    }
    return 0;
}

static int eth_board_close(board_t *board) {
    return 0;
}

static int eth_scan_one_addr(board_access_t *access) {
    lbp16_cmd_addr packet, packet2;
    int send = 0, recv = 0, ret = 0;
    u32 cookie;

    LBP16_INIT_PACKET4(packet, CMD_READ_HM2_COOKIE, HM2_COOKIE_REG);
    send = lbp16_send_packet(&packet, sizeof(packet));
    sleep_ns(2*1000*1000);
    recv = lbp16_recv_packet(&cookie, sizeof(cookie));

    if ((recv > 0) && (cookie == HM2_COOKIE)) {
        char buff[16];
        board_t *board = &boards[boards_count];

        board_init_struct(board);

        eth_socket_blocking();
        LBP16_INIT_PACKET4(packet2, CMD_READ_BOARD_INFO_ADDR16_INCR(8), 0);
        memset(buff, 0, sizeof(buff));
        send = lbp16_send_packet(&packet2, sizeof(packet2));
        recv = lbp16_recv_packet(&buff, sizeof(buff));

        if (strncmp(buff, "7I80DB-16", 9) == 0) {
            board->type = BOARD_ETH;
            strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
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
            board->llio.write_flash = &remote_write_flash;
            board->llio.verify_flash = &remote_verify_flash;
            board->llio.reset = &lbp16_board_reset;
            board->llio.reload = &lbp16_board_reload;

            board->open = &eth_board_open;
            board->close = &eth_board_close;
            board->print_info = &eth_print_info;
            board->flash = BOARD_FLASH_REMOTE;
            board->fallback_support = 1;
            board->llio.verbose = access->verbose;
        } else if (strncmp(buff, "7I80DB-25", 9) == 0) {
            board->type = BOARD_ETH;
            strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
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
            board->llio.write_flash = &remote_write_flash;
            board->llio.verify_flash = &remote_verify_flash;
            board->llio.reset = &lbp16_board_reset;
            board->llio.reload = &lbp16_board_reload;

            board->open = &eth_board_open;
            board->close = &eth_board_close;
            board->print_info = &eth_print_info;
            board->flash = BOARD_FLASH_REMOTE;
            board->fallback_support = 1;
            board->llio.verbose = access->verbose;
        } else if (strncmp(buff, "7I80HD-16", 9) == 0) {
            board->type = BOARD_ETH;
            strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
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
            board->llio.write_flash = &remote_write_flash;
            board->llio.verify_flash = &remote_verify_flash;
            board->llio.reset = &lbp16_board_reset;
            board->llio.reload = &lbp16_board_reload;

            board->open = &eth_board_open;
            board->close = &eth_board_close;
            board->print_info = &eth_print_info;
            board->flash = BOARD_FLASH_REMOTE;
            board->fallback_support = 1;
            board->llio.verbose = access->verbose;
        } else if (strncmp(buff, "7I80HD-25", 9) == 0) {
            board->type = BOARD_ETH;
            strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
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
            board->llio.write_flash = &remote_write_flash;
            board->llio.verify_flash = &remote_verify_flash;
            board->llio.reset = &lbp16_board_reset;
            board->llio.reload = &lbp16_board_reload;

            board->open = &eth_board_open;
            board->close = &eth_board_close;
            board->print_info = &eth_print_info;
            board->flash = BOARD_FLASH_REMOTE;
            board->fallback_support = 1;
            board->llio.verbose = access->verbose;
        } else if (strncmp(buff, "7I76E-16", 9) == 0) {
            board->type = BOARD_ETH;
            strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
            strncpy(board->llio.board_name, buff, 16);
            board->llio.num_ioport_connectors = 3;
            board->llio.pins_per_connector = 17;
            board->llio.ioport_connector_name[0] = "on-card";
            board->llio.ioport_connector_name[1] = "P1";
            board->llio.ioport_connector_name[2] = "P2";
            board->llio.fpga_part_number = "6slx16ftg256";
            board->llio.num_leds = 4;
            board->llio.read = &eth_read;
            board->llio.write = &eth_write;
            board->llio.write_flash = &remote_write_flash;
            board->llio.verify_flash = &remote_verify_flash;
            board->llio.reset = &lbp16_board_reset;
            board->llio.reload = &lbp16_board_reload;

            board->open = &eth_board_open;
            board->close = &eth_board_close;
            board->print_info = &eth_print_info;
            board->flash = BOARD_FLASH_REMOTE;
            board->fallback_support = 1;
            board->llio.verbose = access->verbose;
        } else if (strncmp(buff, "7I92", 4) == 0) {
            board->type = BOARD_ETH;
            strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
            strncpy(board->llio.board_name, buff, 16);
            board->llio.num_ioport_connectors = 2;
            board->llio.pins_per_connector = 17;
            board->llio.ioport_connector_name[0] = "P2";
            board->llio.ioport_connector_name[1] = "P1";
            board->llio.fpga_part_number = "6slx9tqg144";
            board->llio.num_leds = 4;
            board->llio.read = &eth_read;
            board->llio.write = &eth_write;
            board->llio.write_flash = &remote_write_flash;
            board->llio.verify_flash = &remote_verify_flash;
            board->llio.reset = &lbp16_board_reset;
            board->llio.reload = &lbp16_board_reload;

            board->open = &eth_board_open;
            board->close = &eth_board_close;
            board->print_info = &eth_print_info;
            board->flash = BOARD_FLASH_REMOTE;
            board->fallback_support = 1;
            board->llio.verbose = access->verbose;
        } else if (strncmp(buff, "7I93", 4) == 0) {
            board->type = BOARD_ETH;
            strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
            strncpy(board->llio.board_name, buff, 16);
            board->llio.num_ioport_connectors = 2;
            board->llio.pins_per_connector = 24;
            board->llio.ioport_connector_name[0] = "P2";
            board->llio.ioport_connector_name[1] = "P1";
            board->llio.fpga_part_number = "6slx9tqg144";
            board->llio.num_leds = 4;
            board->llio.read = &eth_read;
            board->llio.write = &eth_write;
            board->llio.write_flash = &remote_write_flash;
            board->llio.verify_flash = &remote_verify_flash;
            board->llio.reset = &lbp16_board_reset;
            board->llio.reload = &lbp16_board_reload;

            board->open = &eth_board_open;
            board->close = &eth_board_close;
            board->print_info = &eth_print_info;
            board->flash = BOARD_FLASH_REMOTE;
            board->fallback_support = 1;
            board->llio.verbose = access->verbose;
        } else if (strncmp(buff, "7I96", 4) == 0) {
            board->type = BOARD_ETH;
            strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
            strncpy(board->llio.board_name, buff, 16);
            board->llio.num_ioport_connectors = 3;
            board->llio.pins_per_connector = 13;
            board->llio.ioport_connector_name[0] = "P1";
            board->llio.ioport_connector_name[1] = "TB1";
            board->llio.ioport_connector_name[2] = "TB2";
            board->llio.ioport_connector_name[3] = "TB3";
            board->llio.fpga_part_number = "6slx9tqg144";
            board->llio.num_leds = 4;
            board->llio.read = &eth_read;
            board->llio.write = &eth_write;
            board->llio.write_flash = &remote_write_flash;
            board->llio.verify_flash = &remote_verify_flash;
            board->llio.reset = &lbp16_board_reset;
            board->llio.reload = &lbp16_board_reload;

            board->open = &eth_board_open;
            board->close = &eth_board_close;
            board->print_info = &eth_print_info;
            board->flash = BOARD_FLASH_REMOTE;
            board->fallback_support = 1;
            board->llio.verbose = access->verbose;
        } else {
            printf("Unsupported ethernet device %s at %s\n", buff, eth_socket_get_src_ip());
            ret = -1;
        }
        boards_count++;

        eth_socket_nonblocking();
    }
    return ret;
}

// public functions

int eth_boards_init(board_access_t *access) {
// open socket
#ifdef __linux__
    sd = socket (PF_INET, SOCK_DGRAM, 0);
#elif _WIN32
    int iResult;
    WSADATA wsaData;
    u_long iMode = 1;    // block mode in windows

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return;
    }

    sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sd == 0) {
        printf("Can't open socket: %d %d\n", sd, errno);
        return;
    }
#endif
    src_addr.sin_family = AF_INET;
    dst_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(LBP16_UDP_PORT);
    dst_addr.sin_port = htons(LBP16_UDP_PORT);
    len = sizeof(src_addr);
    lbp16_init(BOARD_ETH);
    return 0;
}

void eth_boards_cleanup(board_access_t *access) {
    int i;

    for (i = 0; i < boards_count; i++) {
        board_t *board = &boards[i];

        eeprom_cleanup(&(board->llio));
    }

    close(sd);
}

void eth_boards_scan(board_access_t *access) {
    char addr[16];
    int i;
    char *ptr;

    if (access->address == 0) {
        access->dev_addr = LBP16_HW_IP;
    }

#ifdef __linux__
    if (inet_pton(AF_INET, access->dev_addr, addr) != 1) {
#elif _WIN32
    struct sockaddr ss;
    int size;
    if (WSAStringToAddress(access->dev_addr, AF_INET, NULL, (struct sockaddr *)&ss, &size) != 0) {
#endif
        return;
    };

    eth_socket_nonblocking();

    if (access->address == 1) {
        eth_socket_set_dest_ip(access->dev_addr);
        eth_scan_one_addr(access);
    } else {
        strncpy(addr, access->dev_addr, 16);
        ptr = strrchr(addr, '.');
        *ptr = '\0';

        for (i = 1; i < 255; i++) {
            char addr_name[32];

            sprintf(addr_name, "%s.%d", addr, i);
            eth_socket_set_dest_ip(addr_name);
            eth_scan_one_addr(access);
        }
    }
    eth_socket_blocking();
}

void eth_print_info(board_t *board) {
    lbp16_cmd_addr packet;
    int i, j;
    u32 flash_id;
    char *mem_types[16] = {NULL, "registers", "memory", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "EEPROM", "flash"};
    char *mem_writeable[2] = {"RO", "RW"};
    char *acc_types[4] = {"8-bit", "16-bit", "32-bit", "64-bit"};
    char *led_debug_types[2] = {"Hostmot2", "eth debug"};
    char *boot_jumpers_types[4] = {"fixed "LBP16_HW_IP, "fixed from EEPROM", "from BOOTP", "INVALID"};
    lbp_mem_info_area mem_area;
    lbp_eth_eeprom_area eth_area;
    lbp_timers_area timers_area;
    lbp_status_area stat_area;
    lbp_info_area info_area;
    lbp16_cmd_addr cmds[LBP16_MEM_SPACE_COUNT];

    printf("\nETH device %s at ip=%s\n", board->llio.board_name, eth_socket_get_src_ip());
    if (board->llio.verbose == 0)
        return;
        
    LBP16_INIT_PACKET4(cmds[0], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_HM2, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[1], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_ETH_CHIP, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[2], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_ETH_EEPROM, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[3], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_FPGA_FLASH, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[4], 0, 0);
    LBP16_INIT_PACKET4(cmds[5], 0, 0);
    LBP16_INIT_PACKET4(cmds[6], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_COMM_CTRL, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[7], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_BOARD_INFO, sizeof(mem_area)/2), 0);

    LBP16_INIT_PACKET4(packet, CMD_READ_ETH_EEPROM_ADDR16_INCR(sizeof(eth_area)/2), 0);
    memset(&eth_area, 0, sizeof(eth_area));
    lbp16_send_packet(&packet, sizeof(packet));
    lbp16_recv_packet(&eth_area, sizeof(eth_area));

    LBP16_INIT_PACKET4(packet, CMD_READ_COMM_CTRL_ADDR16_INCR(sizeof(stat_area)/2), 0);
    memset(&stat_area, 0, sizeof(stat_area));
    lbp16_send_packet(&packet, sizeof(packet));
    lbp16_recv_packet(&stat_area, sizeof(stat_area));

    LBP16_INIT_PACKET4(packet, CMD_READ_BOARD_INFO_ADDR16_INCR(sizeof(info_area)/2), 0);
    memset(&info_area, 0, sizeof(info_area));
    lbp16_send_packet(&packet, sizeof(packet));
    lbp16_recv_packet(&info_area, sizeof(info_area));

    if (info_area.LBP16_version >= 3) {
        LBP16_INIT_PACKET4(cmds[4], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_TIMER, sizeof(mem_area)/2), 0);
        LBP16_INIT_PACKET4(packet, CMD_READ_TIMER_ADDR16_INCR(sizeof(timers_area)/2), 0);
        memset(&timers_area, 0, sizeof(timers_area));
        lbp16_send_packet(&packet, sizeof(packet));
        lbp16_recv_packet(&timers_area, sizeof(timers_area));
    }

    printf("Communication:\n");
    printf("  transport layer: ethernet IPv4 UDP\n");
    printf("  ip address: %s\n", board->dev_addr);
    printf("  mac address: %02X:%02X:%02X:%02X:%02X:%02X\n", HI_BYTE(eth_area.mac_addr_hi), LO_BYTE(eth_area.mac_addr_hi),
      HI_BYTE(eth_area.mac_addr_mid), LO_BYTE(eth_area.mac_addr_mid), HI_BYTE(eth_area.mac_addr_lo), LO_BYTE(eth_area.mac_addr_lo));
    printf("  protocol: LBP16 version %d\n", info_area.LBP16_version);

    show_board_info(board);

    printf("Board firmware info:\n");
    printf("  memory spaces:\n");
    for (i = 0; i < LBP16_MEM_SPACE_COUNT; i++) {
        u32 size;

        if ((cmds[i].cmd_lo == 0) && (cmds[i].cmd_hi == 0)) continue;
        memset(&mem_area, 0, sizeof(mem_area));
        lbp16_send_packet(&cmds[i], sizeof(cmds[i]));
        lbp16_recv_packet(&mem_area, sizeof (mem_area));

        printf("    %d: %.*s (%s, %s", i, (int)sizeof(mem_area.name), mem_area.name, mem_types[(mem_area.size  >> 8) & 0x7F],
          mem_writeable[(mem_area.size & 0x8000) >> 15]);
        for (j = 0; j < 4; j++) {
            if ((mem_area.size & 0xFF) & 1 << j)
                printf(", %s)", acc_types[j]);
        }
        size = pow(2, mem_area.range & 0x3F);
        show_formatted_size(size);
        if (((mem_area.size  >> 8) & 0x7F) >= 0xE)
            printf(", page size: %u, erase size: %u",
              (unsigned int) pow(2, (mem_area.range >> 6) & 0x1F), (unsigned int) pow(2, (mem_area.range >> 11) & 0x1F));
        printf("\n");
    }
 
    printf("  [space 0] HostMot2\n");
    printf("  [space 2] Ethernet eeprom:\n");
    printf("    mac address: %02X:%02X:%02X:%02X:%02X:%02X\n", HI_BYTE(eth_area.mac_addr_hi), LO_BYTE(eth_area.mac_addr_hi),
      HI_BYTE(eth_area.mac_addr_mid), LO_BYTE(eth_area.mac_addr_mid), HI_BYTE(eth_area.mac_addr_lo), LO_BYTE(eth_area.mac_addr_lo));
    printf("    ip address: %u.%u.%u.%u\n", HI_BYTE(eth_area.ip_addr_hi), LO_BYTE(eth_area.ip_addr_hi), HI_BYTE(eth_area.ip_addr_lo), LO_BYTE(eth_area.ip_addr_lo));
    printf("    board name: %.*s\n", (int)sizeof(eth_area.name), eth_area.name);
    printf("    user leds: %s\n", led_debug_types[eth_area.led_debug & 0x1]);

    printf("  [space 3] FPGA flash eeprom:\n");
    lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &flash_id, 4);
    printf("    flash size: %s (id: 0x%02X)\n", eeprom_get_flash_type(flash_id), flash_id);

    if (info_area.LBP16_version >= 3) {
        printf("  [space 4] timers:\n");
        printf("    uSTimeStampReg: 0x%04X\n", timers_area.uSTimeStampReg);
        printf("    WaituSReg: 0x%04X\n", timers_area.WaituSReg);
        printf("    HM2Timeout: 0x%04X\n", timers_area.HM2Timeout);
    }

    printf("  [space 6] LBP16 control/status:\n");
    printf("    packets received: all %d, UDP %d, bad %d\n", stat_area.RXPacketCount, stat_area.RXGoodCount, stat_area.RXBadCount);
    printf("    packets sended: all %d, UDP %d, bad %d\n", stat_area.TXPacketCount, stat_area.TXGoodCount, stat_area.TXBadCount);
    printf("    parse errors: %d, mem errors %d, write errors %d\n", stat_area.LBPParseErrors, stat_area.LBPMemErrors, stat_area.LBPWriteErrors);
    printf("    error flags: 0x%04X\n", stat_area.ErrorReg);
    printf("    debug LED ptr: 0x%04X\n", stat_area.DebugLEDPtr);
    printf("    scratch: 0x%04X\n", stat_area.Scratch);

    printf("  [space 7] LBP16 info:\n");
    printf("    board name: %.*s\n", (int)sizeof(info_area.name), info_area.name);
    printf("    LBP16 protocol version %d\n", info_area.LBP16_version);
    printf("    board firmware version %d\n", info_area.firmware_version);
    printf("    IP address jumpers at boot: %s\n", boot_jumpers_types[info_area.jumpers]);
}

int eth_set_remote_ip(char *ip_addr) {
    lbp16_write_ip_addr_packets write_ip_pck;
    u32 addr;

#ifdef __linux__
    if (inet_pton(AF_INET, ip_addr, &addr) != 1) {
#elif _WIN32
    struct sockaddr ss;
    int size;
    if (WSAStringToAddress(ip_addr, AF_INET, NULL, (struct sockaddr *)&addr, &size) != 0) {
#endif
        printf("Error: invalid format of IP address\n");
        return -EINVAL;
    }
    addr = htonl(addr);
    LBP16_INIT_PACKET6(write_ip_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A02);
    LBP16_INIT_PACKET8(write_ip_pck.eth_write_ip_pck, CMD_WRITE_ETH_EEPROM_ADDR16_INCR(2), ETH_EEPROM_IP_REG, addr);
    sendto (sd, (char*) &write_ip_pck, sizeof(write_ip_pck), 0, (struct sockaddr *) &dst_addr, sizeof(dst_addr));
    return 0;
}
