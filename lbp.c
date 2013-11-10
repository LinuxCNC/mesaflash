
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include <sys/fcntl.h>
#include <unistd.h>

#include "anyio.h"

int sd;

u8 lbp_read_ctrl(u8 cmd) {
    int send, recv;
    u8 data;

    send = write(sd, &cmd, 1);
    recv = read(sd, &data, 1);
    return data;
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

