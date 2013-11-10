
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "anyio.h"
#include "lbp.h"

int sd;

u8 lbp_read_ctrl(u8 cmd) {
    int send, recv;
    u8 data;

    send = write(sd, &cmd, 1);
    recv = read(sd, &data, 1);
    if (LBP_SENDRECV_DEBUG)
        printf("%d=send(%X), %d=recv(%X)\n", send, cmd, recv, data);
    return data;
}

int lbp_read(u16 addr, void *buffer) {
    int send, recv;
    lbp_cmd_addr packet;

    packet.cmd = LBP_CMD_READ | LBP_ARGS_32BIT;
    packet.addr_hi = LO_BYTE(addr);
    packet.addr_lo = HI_BYTE(addr);

    send = write(sd, &packet, sizeof(packet));
    recv = read(sd, &buffer, 4);
    if (LBP_SENDRECV_DEBUG)
        printf("%d=send(), %d=recv()\n", send, recv);
    return 0;
}

int lbp_write(u16 addr, void *buffer) {
    int send, recv;
    lbp_cmd_addr_data packet;

    packet.cmd = LBP_CMD_WRITE | LBP_ARGS_32BIT;
    packet.addr_hi = LO_BYTE(addr);
    packet.addr_lo = HI_BYTE(addr);
    memcpy(&packet.data, buffer, 4);

    send = write(sd, &packet, sizeof(lbp_cmd_addr_data));
    if (LBP_SENDRECV_DEBUG)
        printf("%d=send(), %d=recv()\n", send, recv);
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
#endif
}

void lbp_release() {
#ifdef __linux__
    close(sd);
#endif
}
