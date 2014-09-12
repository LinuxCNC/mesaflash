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

#include "types.h"

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "eeprom_local.h"
#include "boards.h"

int sd = -1;
struct spi_ioc_transfer settings;

static u32 read_command(u16 addr, unsigned nelem) {
    bool increment = true;
    return (addr << 16) | 0xA000 | (increment ? 0x800 : 0) | (nelem << 4);
}

static u32 write_command(u16 addr, unsigned nelem) {
    bool increment = true;
    return (addr << 16) | 0xB000 | (increment ? 0x800 : 0) | (nelem << 4);
}


int spilbp_write(u16 addr, void *buffer, int size) {
    if(size % 4 != 0) return -1;
    u32 txbuf[1+size/4];
    txbuf[0] = write_command(addr, size/4);
    memcpy(txbuf + 1, buffer, size);

    struct spi_ioc_transfer t;
    t = settings;
    t.tx_buf = (uint64_t)(intptr_t)txbuf;
    t.len = sizeof(txbuf);

    int r = ioctl(sd, SPI_IOC_MESSAGE(1), &t);
    if(r <= 0) return r;
    return 0;
}

int spilbp_read(u16 addr, void *buffer, int size) {
    if(size % 4 != 0) return -1;
    u32 trxbuf[1+size/4];
    trxbuf[0] = read_command(addr, size/4);
    memset(trxbuf+1, 0, size);

    struct spi_ioc_transfer t;
    t = settings;
    t.tx_buf = t.rx_buf = (uint64_t)(intptr_t)trxbuf;
    t.len = sizeof(trxbuf);

    int r = ioctl(sd, SPI_IOC_MESSAGE(1), &t);
    if(r < 0) return r;

    memcpy(buffer, trxbuf+1, size);
    return 0;
}

void spilbp_print_info() {
}

static int spidev_set_lsb_first(int fd, u8 lsb_first) {
    return ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb_first);
}

static int spidev_set_mode(int fd, u8 mode) {
    return ioctl(fd, SPI_IOC_WR_MODE, &mode);
}

static int spidev_set_max_speed_hz(int fd, u32 speed_hz) {
    return ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz);
}

static int spidev_set_bits_per_word(int fd, u8 bits) {
    return ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
}


void spilbp_init(board_access_t *access) {
    settings.speed_hz = 8 * 1000 * 1000;
    settings.bits_per_word = 32;

    sd = open(access->dev_addr, O_RDWR);
    if(sd == -1) {
        perror("open");
        return;
    }
    spidev_set_lsb_first(sd, false);
    spidev_set_mode(sd, 0);
    spidev_set_bits_per_word(sd, 32);
    spidev_set_max_speed_hz(sd, settings.speed_hz);
}

void spilbp_release() {
    if(sd != -1) close(sd);
}
