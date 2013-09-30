
#include <pci/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "lbp16.h"

#ifdef __linux__
    extern int sd;
    extern socklen_t len;
#elif _WIN32
    extern SOCKET sd;
    extern int len;
#endif
extern struct sockaddr_in server_addr, client_addr;

u32 lbp16_send_read_u16(u16 cmd, u16 addr) {
    lbp16_cmd_addr packet;
    u16 buff = 0;

    LBP16_INIT_PACKET4(packet, cmd, addr);
    if (LBP16_SENDRECV_DEBUG)
        printf("SEND: %02X %02X %02X %02X\n", packet.cmd_hi, packet.cmd_lo, packet.addr_hi, packet.addr_lo);
    sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    recvfrom(sd, (char*) &buff, sizeof(buff), 0, (struct sockaddr *) &client_addr, &len);
    if (LBP16_SENDRECV_DEBUG)
        printf("RECV: %04X\n", buff);

    return buff;
}

void lbp16_send_write_u16(u16 cmd, u16 addr, u16 val) {
    lbp16_cmd_addr_data16 packet;

    LBP16_INIT_PACKET6(packet, cmd, addr, val);
    if (LBP16_SENDRECV_DEBUG)
        printf("SEND: %02X %02X %02X %02X %02X %02X\n", packet.cmd_hi, packet.cmd_lo, packet.addr_hi, packet.addr_lo, packet.data_hi, packet.data_lo);
    sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
}

u32 lbp16_send_read_u32(u16 cmd, u16 addr) {
    lbp16_cmd_addr packet;
    u32 buff = 0;

    LBP16_INIT_PACKET4(packet, cmd, addr);
    if (LBP16_SENDRECV_DEBUG)
        printf("SEND: %02X %02X %02X %02X\n", packet.cmd_hi, packet.cmd_lo, packet.addr_hi, packet.addr_lo);
    sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    recvfrom(sd, (char*) &buff, sizeof(buff), 0, (struct sockaddr *) &client_addr, &len);
    if (LBP16_SENDRECV_DEBUG)
        printf("RECV: %08X\n", (unsigned int) buff);

    return buff;
}

void lbp16_send_write_u32(u16 cmd, u16 addr, u32 val) {
    lbp16_cmd_addr_data32 packet;

    LBP16_INIT_PACKET8(packet, cmd, addr, val);
    if (LBP16_SENDRECV_DEBUG)
        printf("SEND: %02X %02X %02X %02X %02X %02X %02X %02X\n", packet.cmd_hi, packet.cmd_lo, packet.addr_hi, packet.addr_lo, 
          packet.data1, packet.data1, packet.data2, packet.data3);
    sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
}
