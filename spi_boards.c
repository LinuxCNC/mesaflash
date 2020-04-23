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

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "eeprom_local.h"
#include "spi_boards.h"
#include "common.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;

static bool canDo32 = true;

static int sd = -1;
struct spi_ioc_transfer settings;

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

static u32 read_command(u16 addr, unsigned nelem) {
    bool increment = true;
    return (addr << 16) | SPILBP_CMD_READ | (increment ? SPILBP_ADDR_AUTO_INC : 0) | (nelem << 4);
}

static u32 write_command(u16 addr, unsigned nelem) {
    bool increment = true;
    return (addr << 16) | SPILBP_CMD_WRITE | (increment ? SPILBP_ADDR_AUTO_INC : 0) | (nelem << 4);
}

static int spi_board_open(board_t *board) {
    eeprom_init(&(board->llio));
    board->flash_id = read_flash_id(&(board->llio));
    if (board->fallback_support == 1) {
        eeprom_prepare_boot_block(board->flash_id);
        board->flash_start_address = eeprom_calc_user_space(board->flash_id);
    } else {
        board->flash_start_address = 0;
    }
    return 0;
}

static int spi_board_close(board_t *board) {
    return 0;
}

int spi_boards_init(board_access_t *access) {
    settings.speed_hz = 8 * 1000 * 1000;
    settings.bits_per_word = 32;

    sd = open(access->dev_addr, O_RDWR);
    if(sd == -1) {
        perror("open");
        return -1;
    }
    spidev_set_lsb_first(sd, false);
    spidev_set_mode(sd, 0);
    if (spidev_set_bits_per_word(sd, settings.bits_per_word) < 0)
    {
        fprintf(stderr,"unable to set bpw32, fallback to bpw8\n");
        canDo32 = false;
        settings.bits_per_word = 8;
        spidev_set_bits_per_word(sd, settings.bits_per_word);        
    }
    spidev_set_max_speed_hz(sd, settings.speed_hz);

    return 0;
}

void spi_boards_cleanup(board_access_t *access) {
    if(sd != -1) close(sd);
}

void reorderBuffer(char *pBuf, int numInts)
{
    int lcv;
    for (lcv = 0; lcv < numInts; lcv++)
    {
        pBuf[0] ^= pBuf[3];
        pBuf[3] ^= pBuf[0];
        pBuf[0] ^= pBuf[3];
        pBuf[1] ^= pBuf[2];
        pBuf[2] ^= pBuf[1];
        pBuf[1] ^= pBuf[2];
        pBuf += 4;
    }
}

int spi_read(llio_t *self, u32 addr, void *buffer, int size) {
    if(size % 4 != 0) return -1;
    int numInts = 1+size/4;
    u32 trxbuf[numInts];

    trxbuf[0] = read_command(addr, size/4);
    memset(trxbuf+1, 0, size);
    struct spi_ioc_transfer t;
    t = settings;
    t.tx_buf = t.rx_buf = (uint64_t)(intptr_t)trxbuf;
    t.len = sizeof(trxbuf);
    if (!canDo32)
    {
        reorderBuffer((char *) trxbuf, 1);
    }

    int r = ioctl(sd, SPI_IOC_MESSAGE(1), &t);

    if(r < 0) return r;

    if (!canDo32)
    {
        reorderBuffer((char *) trxbuf, numInts);
    }

    memcpy(buffer, trxbuf+1, size);
    return 0;
}

int spi_write(llio_t *self, u32 addr, void *buffer, int size) {
    if(size % 4 != 0) return -1;
    int numInts = 1+size/4;
    u32 txbuf[numInts];

    txbuf[0] = write_command(addr, size/4);
    memcpy(txbuf + 1, buffer, size);

    struct spi_ioc_transfer t;
    t = settings;
    t.tx_buf = (uint64_t)(intptr_t)txbuf;
    t.len = sizeof(txbuf);
    if (!canDo32)
    {
        reorderBuffer((char *) txbuf, numInts);
    }

    int r = ioctl(sd, SPI_IOC_MESSAGE(1), &t);
    if(r <= 0) return r;
    return 0;
}

void spi_boards_scan(board_access_t *access) {
    u32 buf[4];
    u32 cookie[] = {0x55aacafe, 0x54534f48, 0x32544f4d};
    board_t *board = &boards[boards_count];

    board_init_struct(board);
    if (spi_read(&(board->llio), HM2_COOKIE_REG, &buf, sizeof(buf)) < 0) {
        return;
    }

    if(memcmp(buf, cookie, sizeof(cookie))) {
        fprintf(stderr, "Unexpected cookie at %04x..%04lx:\n%08x %08x %08x\n",
            HM2_COOKIE_REG, HM2_COOKIE_REG + sizeof(buf),
            buf[0], buf[1], buf[2]);
        return;
    }

    char ident[8];
    if(spi_read(&(board->llio), buf[3] + 0xc, &ident, sizeof(ident)) < 0) return;
    
    if(!memcmp(ident, "MESA7I90", 8)) {
        board_t *board = &boards[boards_count];
        board->type = BOARD_SPI;
        strcpy(board->dev_addr, access->dev_addr);
        strncpy(board->llio.board_name, "7I90", 4);
        board->llio.num_ioport_connectors = 24;
        board->llio.ioport_connector_name[0] = "P1";
        board->llio.ioport_connector_name[1] = "P2";
        board->llio.ioport_connector_name[2] = "P3";
        board->llio.num_leds = 2;
        board->llio.verbose = access->verbose;
        board->llio.write = spi_write;
        board->llio.read = spi_read;
        board->llio.write_flash = local_write_flash;
        board->llio.verify_flash = local_verify_flash;
        board->llio.fpga_part_number = "6slx9tqg144";
        board->open = spi_board_open;
        board->close = spi_board_close;
        board->print_info = spi_print_info;
        board->flash = BOARD_FLASH_HM2;
        board->fallback_support = 1;
        boards_count ++;
    } else {
        int i=0;
        for(i=0; i<sizeof(ident); i++)
            if(!isprint(ident[i])) ident[i] = '?';

        fprintf(stderr, "Unknown board: %.8s\n", ident);
    }
}

void spi_print_info(board_t *board) {
}
