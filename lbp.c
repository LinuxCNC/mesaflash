
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#include <windows.h>
#endif

#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "anyio.h"
#include "lbp.h"

#ifdef __linux__
int sd;
#elif _WIN32
HANDLE sd;
#endif

int lbp_send(void *packet, int size) {
#ifdef __linux__
    return write(sd, packet, size);
#elif _WIN32
    DWORD send = 0;
	WriteFile(sd, packet, size, &send, NULL);
	return send;
#endif
}

int lbp_recv(void *packet, int size) {
#ifdef __linux__
    return read(sd, packet, size);
#elif _WIN32
    DWORD recv = 0;
	ReadFile(sd, packet, size, &recv, NULL);
	return recv;
#endif
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

    packet.cmd = LBP_CMD_READ | LBP_ARGS_32BIT;
    packet.addr_hi = LO_BYTE(addr);
    packet.addr_lo = HI_BYTE(addr);

    send = lbp_send(&packet, sizeof(lbp_cmd_addr));
    recv = lbp_recv(&buffer, 4);
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

    send = lbp_send(&packet, sizeof(lbp_cmd_addr_data));
    if (LBP_SENDRECV_DEBUG)
        printf("%d=send()\n", send);
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
