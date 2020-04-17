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

#ifdef _WIN32
#include <windows.h>
#endif
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "boards.h"
#include "lbp.h"

#ifdef __linux__
int sd;
#elif _WIN32
HANDLE sd;
#endif

int lbp_send(void *packet, int size) {
#ifdef __linux__
	int send = write(sd, packet, size);
#elif _WIN32
    DWORD send = 0;
	WriteFile(sd, packet, size, &send, NULL);
#endif
    if (LBP_SENDRECV_DEBUG)
        printf("%d=lbp_send(%d)\n", send, size);

    return send;
}

int lbp_recv(void *packet, int size) {
#ifdef __linux__
    int recv = read(sd, packet, size);
#elif _WIN32
    DWORD recv = 0;
	ReadFile(sd, packet, size, &recv, NULL);
#endif
    if (LBP_SENDRECV_DEBUG)
        printf("%d=lbp_recv(%d)\n", recv, size);

    return recv;
}

u8 lbp_read_ctrl(u8 cmd) {
    u8 data;
    int send, recv;

    send = lbp_send(&cmd, 1);
    recv = lbp_recv(&data, 1);
    if (LBP_SENDRECV_DEBUG)
        printf("%d=send(%X), %d=recv(%X)\n", send, cmd, recv, data);

    return data;
}

int lbp_read(u16 addr, void *buffer) {
    int send, recv;
    lbp_cmd_addr packet;
    u32 *ptr = buffer;

    packet.cmd = LBP_CMD_READ | LBP_ARGS_32BIT;
    packet.addr_hi = LO_BYTE(addr);
    packet.addr_lo = HI_BYTE(addr);

    send = lbp_send(&packet, sizeof(lbp_cmd_addr));
    recv = lbp_recv(buffer, 4);
    if (LBP_SENDRECV_DEBUG)
        printf("lbp_read(%02X:%04X): %08X\n", packet.cmd, addr, *ptr);

    return 0;
}

int lbp_write(u16 addr, void *buffer) {
    int send;
    lbp_cmd_addr_data packet;

    packet.cmd = LBP_CMD_WRITE | LBP_ARGS_32BIT;
    packet.addr_hi = LO_BYTE(addr);
    packet.addr_lo = HI_BYTE(addr);
    memcpy(&packet.data, buffer, 4);

    send = lbp_send(&packet, sizeof(lbp_cmd_addr_data));
    if (LBP_SENDRECV_DEBUG)
        printf("lbp_write(%02X:%04X)\n", packet.cmd, addr);

    return 0;
}

void lbp_print_info() {
    u8 data;

    data = lbp_read_ctrl(LBP_CMD_READ_VERSION);
    printf("  LBP version %d\n", data);
}

void lbp_init(board_access_t *access) {
#ifdef __linux__
    sd = open(access->dev_addr, O_RDWR, O_NONBLOCK);
    if (sd == -1) 
        perror("Unable to open the serial port\n");
#elif _WIN32
    char dev_name[32];
    
    snprintf(dev_name, 32, "\\\\.\\%s", access->dev_addr);
    sd = CreateFile(dev_name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (sd == INVALID_HANDLE_VALUE) { 
        printf("Unable to open the serial port %d\n", errno);
    }
#endif
}

void lbp_release() {
#ifdef __linux__
    close(sd);
#endif
}
