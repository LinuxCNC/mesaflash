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

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "types.h"
#include "lbp16.h"
#include "eth_boards.h"
#include "serial_boards.h"

static lbp16_access_t lbp16_access;

int lbp16_send_packet(void *packet, int size) {
    return lbp16_access.send_packet(packet, size);
}

int lbp16_recv_packet(void *packet, int size) {
    return lbp16_access.recv_packet(packet, size);
}

int lbp16_read(u16 cmd, u32 addr, void *buffer, int size) {
    lbp16_cmd_addr packet;
    int send, recv;
    u8 local_buff[size];

    LBP16_INIT_PACKET4(packet, cmd, addr);
    if (LBP16_SENDRECV_DEBUG) {
        printf("SEND: %02X %02X %02X %02X | REQUEST %d bytes\n", packet.cmd_hi, packet.cmd_lo, packet.addr_hi, packet.addr_lo, size);
    }
    send = lbp16_access.send_packet(&packet, sizeof(packet));
    recv = lbp16_access.recv_packet(&local_buff, sizeof(local_buff));
    if (LBP16_SENDRECV_DEBUG) {
        printf("RECV: %d bytes\n", recv);
    }
    memcpy(buffer, local_buff, size);

    return 0;
}

int lbp16_write(u16 cmd, u32 addr, void *buffer, int size) {
    static struct {
        lbp16_cmd_addr wr_packet;
        u8 tmp_buffer[LBP16_MAX_PACKET_DATA_SIZE*8];
    } packet;
    int send;

    LBP16_INIT_PACKET4(packet.wr_packet, cmd, addr);
    memcpy(&packet.tmp_buffer, buffer, size);
    if (LBP16_SENDRECV_DEBUG) {
        printf("SEND: %02X %02X %02X %02X | WRITE %d bytes\n", packet.wr_packet.cmd_hi, packet.wr_packet.cmd_lo,
          packet.wr_packet.addr_hi, packet.wr_packet.addr_lo, size);
    }
    send = lbp16_access.send_packet(&packet, sizeof(lbp16_cmd_addr) + size);

    return 0;
}

int lbp16_board_reset(llio_t *self) {
    int send;
    lbp16_cmd_addr_data16 packet;

    LBP16_INIT_PACKET6(packet, CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1C, 0x0001);   // reset if != 0
    send = lbp16_send_packet(&packet, sizeof(packet));

    return 0;
}

int lbp16_board_reload(llio_t *self, int fallback_flag) {
    board_t *board = self->board;
    int send;
    u32 boot_addr;
    u16 fw_ver;
    lbp16_cmd_addr_data16 packet[14];
    lbp16_cmd_addr fw_packet;

    LBP16_INIT_PACKET4(fw_packet, CMD_READ_BOARD_INFO_ADDR16(1), offsetof(lbp_info_area, firmware_version));
    lbp16_send_packet(&fw_packet, sizeof(fw_packet));
    lbp16_recv_packet(&fw_ver, sizeof(fw_ver));

    if ((board->type & BOARD_ETH) && (fw_ver < 15)) {
        printf("ERROR: FPGA reload only supported by ethernet card firmware > 14.\n");
        return -1;
    } else if ((board->type & BOARD_SER) && (fw_ver < 2)) {
        printf("ERROR: FPGA reload only supported by serial card firmware > 2.\n");
        return -1;
    }

    if (fallback_flag == 1) {
        boot_addr = 0x10000;
    } else {
        boot_addr = 0x0;
    }
    boot_addr |= 0x0B000000; // plus read command in high byte

    LBP16_INIT_PACKET6(packet[0], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0xFFFF);   // dummy
    LBP16_INIT_PACKET6(packet[1], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0xFFFF);   // dummy
    LBP16_INIT_PACKET6(packet[2], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0xAA99);   // sync
    LBP16_INIT_PACKET6(packet[3], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x5566);   // sync
    LBP16_INIT_PACKET6(packet[4], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x3261);   // load low flash start address
    LBP16_INIT_PACKET6(packet[5], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, boot_addr & 0xFFFF);   // start addr
    LBP16_INIT_PACKET6(packet[6], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x3281);   // load high start address + read command
    LBP16_INIT_PACKET6(packet[7], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, boot_addr >> 16);   // start addr (plus read command in high byte)
    LBP16_INIT_PACKET6(packet[8], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x30A1);   // load command register
    LBP16_INIT_PACKET6(packet[9], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x000E);   // IPROG command
    LBP16_INIT_PACKET6(packet[10], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x2000);  // NOP
    LBP16_INIT_PACKET6(packet[11], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x2000);  // NOP
    LBP16_INIT_PACKET6(packet[12], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x2000);  // NOP
    LBP16_INIT_PACKET6(packet[13], CMD_WRITE_COMM_CTRL_ADDR16(1), 0x1E, 0x2000);  // NOP
    send = lbp16_send_packet(&packet, sizeof(packet));

    return 0;
}

void lbp16_init(int board_type) {
    switch (board_type) {
        case BOARD_ETH:
            lbp16_access.send_packet = &eth_send_packet;
            lbp16_access.recv_packet = &eth_recv_packet;
            break;
        case BOARD_SER:
            lbp16_access.send_packet = &serial_send_packet;
            lbp16_access.recv_packet = &serial_recv_packet;
            break;
        default:
            break;
    }
}

void lbp_cleanup(int board_type) {
}
