
#ifdef __linux__
#include <pci/pci.h>
#include <sys/io.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <stdio.h>

#include "spi_access_io.h"
#include "hostmot2.h"
#include "anyio.h"

static void wait_for_data(llio_t *self) {
    board_t *board = self->private;
    u32 i = 0;
    u8 data = 0;

    for (i = 0; (((data & 0xFF) & DAV_MASK) == 0) && (i < 5000) ; i++) {
        data = inb(board->data_base_addr + SPI_CS_OFFSET);
    }
    if (i == 5000) {
        printf("%x timeout waiting for SPI data\n", data);
    }
}

static void set_cs_high(llio_t *self) {
    board_t *board = self->private;

    outb(1, board->data_base_addr + SPI_CS_OFFSET);
}

static void set_cs_low(llio_t *self) {
    board_t *board = self->private;

    outb(0, board->data_base_addr + SPI_CS_OFFSET);
}

static void prefix(llio_t *self) {
    set_cs_low(self);
}

static void suffix(llio_t *self) {
    set_cs_high(self);
}

static void send_byte(llio_t *self, u8 byte) {
    board_t *board = self->private;

    outb(byte, board->data_base_addr + SPI_SREG_OFFSET);
}

static u8 recv_byte(llio_t *self) {
    board_t *board = self->private;

    outb(0, board->data_base_addr + SPI_SREG_OFFSET);
    wait_for_data(self);
    return inb(board->data_base_addr + SPI_SREG_OFFSET);
}

void open_spi_access_io(llio_t *self, spi_eeprom_dev_t *access) {
    access->set_cs_low = &set_cs_low, 
    access->set_cs_high = &set_cs_high;
    access->prefix = &prefix;
    access->suffix = &suffix;
    access->send_byte = &send_byte;
    access->recv_byte = &recv_byte;
};

void close_spi_access_io(llio_t *self, spi_eeprom_dev_t *access) {
}
