
#include <pci/pci.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "usb_boards.h"
#include "common.h"
#include "anyio.h"
#include "spi_eeprom.h"
#include "bitfile.h"

void usb_boards_init() {
}

void usb_boards_scan() {
    int sd, i;
    struct termios options;
    u8 cmd = 0x1;
    u32 data = 0x1234;

    sd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NONBLOCK);

    if(sd == -1){perror("Unable to open the serial port\n");}
    printf("Serial port open successful\n");

    printf("Reading serial port ...\n\n"); 
    i = write(sd, &cmd, 1);
    printf("i=%d\n", i);

    i = read(sd, &data, 1);
    printf("i=%d 0x%04X %d\n", i, data, errno);
    close(sd);
}
