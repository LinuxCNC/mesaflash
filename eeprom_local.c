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

#ifdef __linux__
#if MESAFLASH_IO
#include <sys/io.h>
#endif
#include <pci/pci.h>
#elif _WIN32
#include <windows.h>
#include "libpci/pci.h"
#endif
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "types.h"
#include "eeprom.h"
#include "eeprom_local.h"
#include "hostmot2.h"
#include "bitfile.h"
#include "epp_boards.h"

extern u8 boot_block[BOOT_BLOCK_SIZE];
extern u8 page_buffer[PAGE_SIZE];
extern u8 file_buffer[SECTOR_SIZE];

extern spi_eeprom_dev_t eeprom_access;

// spi access via hm2 registers

static void wait_for_data_hm2(llio_t *self) {
    u32 i = 0;
    u32 data = 0;

    for (i = 0; (((data & 0xFF) & SPI_DAV_MASK) == 0) && (i < 5000) ; i++) {
        self->read(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
    }
    if (i == 5000) {
        printf("%x timeout waiting for SPI data\n", data);
    }
}

static void set_cs_high_hm2(llio_t *self) {
    u32 data = 1;

    self->write(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
}

static void set_cs_low_hm2(llio_t *self) {
    u32 data = 0;

    self->write(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
}

static void prefix_hm2(llio_t *self) {
    set_cs_low_hm2(self);
}

static void suffix_hm2(llio_t *self) {
    set_cs_high_hm2(self);
}

static void send_byte_hm2(llio_t *self, u8 byte) {
    u32 data = byte;

    self->write(self, HM2_SPI_DATA_REG, &data, sizeof(data));
    wait_for_data_hm2(self);
}

static u8 recv_byte_hm2(llio_t *self) {
    u32 data = 0;
    u32 recv = 0;
    
    self->write(self, HM2_SPI_DATA_REG, &data, sizeof(data));
    wait_for_data_hm2(self);
    self->read(self, HM2_SPI_DATA_REG, &recv, sizeof(recv));
    return (u8) recv & 0xFF;
}

// spi access via gpio pins of xio2001 bridge on 6i25

static u16 GPIO_reg_val;

static void set_cs_high_gpio(llio_t *self) {
    board_t *board = self->board;

    GPIO_reg_val = GPIO_reg_val | 0x2;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_cs_low_gpio(llio_t *self) {
    board_t *board = self->board;

    GPIO_reg_val = GPIO_reg_val & (~ 0x2);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_din_high_gpio(llio_t *self) {
    board_t *board = self->board;

    GPIO_reg_val = GPIO_reg_val | 0x8;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_din_low_gpio(llio_t *self) {
    board_t *board = self->board;

    GPIO_reg_val = GPIO_reg_val & (~0x8);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_clock_high_gpio(llio_t *self) {
    board_t *board = self->board;

    GPIO_reg_val = GPIO_reg_val | 0x10;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_clock_low_gpio(llio_t *self) {
    board_t *board = self->board;

    GPIO_reg_val = GPIO_reg_val & (~ 0x10);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static int get_bit_gpio(llio_t *self) {
    board_t *board = self->board;
    u16 data;

    data = pci_read_word(board->dev, XIO2001_GPIO_DATA_REG);
    if (data & 0x4)
        return 1;
    else
        return 0;
}

static void prefix_gpio(llio_t *self) {
    set_cs_high_gpio(self);
    set_din_low_gpio(self);
    set_clock_low_gpio(self);
    set_cs_low_gpio(self);
}

static void suffix_gpio(llio_t *self) {
    set_cs_high_gpio(self);
    set_din_low_gpio(self);
}

static void init_gpio(llio_t *self) {
    board_t *board = self->board;

    pci_write_word(board->dev, XIO2001_GPIO_ADDR_REG, 0x001B);
    pci_write_word(board->dev, XIO2001_SBAD_STAT_REG, 0x0000);
    GPIO_reg_val = 0x0003;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void restore_gpio(llio_t *self) {
    board_t *board = self->board;

    GPIO_reg_val = 0x0003;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
    pci_write_word(board->dev, XIO2001_GPIO_ADDR_REG, 0x0000);
}

static void send_byte_gpio(llio_t *self, u8 byte) {
    u32 mask = DATA_MASK;
    int i;

    for (i = 0; i < CMD_LEN; i++) {
        if ((mask & byte) == 0)
            set_din_low_gpio(self);
        else
            set_din_high_gpio(self);
        mask >>= 1;
        set_clock_high_gpio(self);
        set_clock_low_gpio(self);
    }
}

static u8 recv_byte_gpio(llio_t *self) {
    u32 mask, data = 0;
    int i;

    mask = DATA_MASK;
    for (i = 0; i < DATA_LEN; i++) {
        if (get_bit_gpio(self) == 1)
            data |= mask;
        mask >>= 1;
        set_clock_high_gpio(self);
        set_clock_low_gpio(self);
    }
  return data;
}

// spi access via io ports like on 3x20

#if MESAFLASH_IO
static void wait_for_data_io(llio_t *self) {
    board_t *board = self->board;
    u32 i = 0;
    u8 data = 0;

    for (i = 0; (((data & 0xFF) & SPI_DAV_MASK) == 0) && (i < 5000) ; i++) {
        data = inb(board->data_base_addr + IO_SPI_CS_OFFSET);
    }
    if (i == 5000) {
        printf("%x timeout waiting for SPI data\n", data);
    }
}

static void set_cs_high_io(llio_t *self) {
    board_t *board = self->board;

    outb(1, board->data_base_addr + IO_SPI_CS_OFFSET);
}

static void set_cs_low_io(llio_t *self) {
    board_t *board = self->board;

    outb(0, board->data_base_addr + IO_SPI_CS_OFFSET);
}

static void prefix_io(llio_t *self) {
    set_cs_low_io(self);
}

static void suffix_io(llio_t *self) {
    set_cs_high_io(self);
}

static void send_byte_io(llio_t *self, u8 byte) {
    board_t *board = self->board;

    outb(byte, board->data_base_addr + IO_SPI_SREG_OFFSET);
}

static u8 recv_byte_io(llio_t *self) {
    board_t *board = self->board;

    outb(0, board->data_base_addr + IO_SPI_SREG_OFFSET);
    wait_for_data_io(self);
    return inb(board->data_base_addr + IO_SPI_SREG_OFFSET);
}

// spi access via epp on board 7i43 with EPPIO firmware

void wait_for_data_epp(llio_t *self) {
    board_t *board = self->board;
    u32 i = 0;
    u8 data = 0;

    epp_addr8(board, EPP_SPI_CS_REG);
    for (i = 0; ((data & SPI_DAV_MASK) == 0) && (i < 100) ; i++) {
        data = epp_read8(board);
    }
    if (i == 100) {
        printf("%x timeout waiting for SPI data\n", data);
    }
}

static void set_cs_high_epp(llio_t *self) {
    board_t *board = self->board;

    epp_addr8(board, EPP_SPI_CS_REG);
    epp_write8(board, 1);
}

static void set_cs_low_epp(llio_t *self) {
    board_t *board = self->board;

    epp_addr8(board, EPP_SPI_CS_REG);
    epp_write8(board, 0);
}

static void prefix_epp(llio_t *self) {
    set_cs_low_epp(self);
}

static void suffix_epp(llio_t *self) {
    set_cs_high_epp(self);
}

static void send_byte_epp(llio_t *self, u8 byte) {
    board_t *board = self->board;

    epp_addr8(board, EPP_SPI_SREG_REG);
    epp_write8(board, byte);
}

static u8 recv_byte_epp(llio_t *self) {
    board_t *board = self->board;

    epp_addr8(board, EPP_SPI_SREG_REG);
    epp_write8(board, 0);
    wait_for_data_epp(self);
    epp_addr8(board, EPP_SPI_SREG_REG);
    return epp_read8(board);
}
#endif

// pci flash

static void send_address(llio_t *self, u32 addr) {
    eeprom_access.send_byte(self, (addr >> 16) & 0xFF);
    eeprom_access.send_byte(self, (addr >> 8) & 0xFF);
    eeprom_access.send_byte(self, addr & 0xFF);
}

static void write_enable(llio_t *self) {
    eeprom_access.prefix(self);
    eeprom_access.send_byte(self, SPI_CMD_WRITE_ENABLE);
    eeprom_access.suffix(self);
}

static u8 read_status(llio_t *self) {
    u8 ret;

    eeprom_access.prefix(self);
    eeprom_access.send_byte(self, SPI_CMD_READ_STATUS);
    ret = eeprom_access.recv_byte(self);
    eeprom_access.suffix(self);
    return ret;
}

u8 read_flash_id(llio_t *self) {
    u8 ret;

    eeprom_access.prefix(self);
    eeprom_access.send_byte(self, SPI_CMD_READ_IDROM);
    send_address(self, 0); // three dummy bytes
    ret = eeprom_access.recv_byte(self);
    eeprom_access.suffix(self);
    return ret;
}

static void wait_for_write(llio_t *self) {
    u8 stat = read_status(self);

    while ((stat & WRITE_IN_PROGRESS_MASK) != 0) {
         stat = read_status(self);
    }
}

// eeprom access functions

static void read_page(llio_t *self, u32 addr, void *buff) {
    int i;

    eeprom_access.prefix(self);
    eeprom_access.send_byte(self, SPI_CMD_READ);
    send_address(self, addr);
    for (i = 0; i < PAGE_SIZE; i++, buff++) {
        *((u8 *) buff) = eeprom_access.recv_byte(self);
    }
    eeprom_access.suffix(self);
}

static void write_page(llio_t *self, u32 addr, void *buff) {
    int i;
    u8 byte;

    write_enable(self);
    eeprom_access.prefix(self);
    eeprom_access.send_byte(self, SPI_CMD_PAGE_WRITE);
    send_address(self, addr);// { note that add 0..7 should be 0}
    for (i = 0; i < PAGE_SIZE; i++) {
        byte = *(u8 *) (buff + i); 
        eeprom_access.send_byte(self, byte);
    }
    eeprom_access.suffix(self);
    wait_for_write(self);
}

static void erase_sector(llio_t *self, u32 addr) {
    write_enable(self);
    eeprom_access.prefix(self);
    eeprom_access.send_byte(self, SPI_CMD_SECTOR_ERASE);
    send_address(self, addr);
    eeprom_access.suffix(self);
    wait_for_write(self);
}

static int local_start_programming(llio_t *self, u32 start_address, int fsize) {
    int ret;

    eeprom_access.set_cs_high(self);
    ret = start_programming(self, start_address, fsize);
    eeprom_access.set_cs_high(self);
    return ret;
}

static void done_programming(llio_t *self) {
    eeprom_access.set_cs_high(self);
}

// global functions

int local_write_flash(llio_t *self, char *bitfile_name, u32 start_address, int fix_boot_flag, int sha256_check_flag) {
    int ret;

    ret = eeprom_write(self, bitfile_name, start_address, fix_boot_flag, sha256_check_flag);
    done_programming(self);
    return ret;
}

int local_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    return eeprom_verify(self, bitfile_name, start_address);
}

int local_backup_flash(llio_t *self, char *bitfile_name) {
    return flash_backup(self, bitfile_name);
}

int local_restore_flash(llio_t *self, char *bitfile_name) {
    return flash_restore(self, bitfile_name);
}

void open_spi_access_local(llio_t *self) {
    board_t *board = self->board;

    switch (board->flash) {
        case BOARD_FLASH_NONE:
            break;
        case BOARD_FLASH_HM2:
            eeprom_access.set_cs_low = &set_cs_low_hm2;
            eeprom_access.set_cs_high = &set_cs_high_hm2;
            eeprom_access.prefix = &prefix_hm2;
            eeprom_access.suffix = &suffix_hm2;
            eeprom_access.send_byte = &send_byte_hm2;
            eeprom_access.recv_byte = &recv_byte_hm2;
            break;
        case BOARD_FLASH_IO:
#if MESAFLASH_IO
            eeprom_access.set_cs_low = &set_cs_low_io;
            eeprom_access.set_cs_high = &set_cs_high_io;
            eeprom_access.prefix = &prefix_io;
            eeprom_access.suffix = &suffix_io;
            eeprom_access.send_byte = &send_byte_io;
            eeprom_access.recv_byte = &recv_byte_io;
#endif
            break;
        case BOARD_FLASH_GPIO:
            eeprom_access.set_cs_low = &set_cs_low_gpio;
            eeprom_access.set_cs_high = &set_cs_high_gpio;
            eeprom_access.prefix = &prefix_gpio;
            eeprom_access.suffix = &suffix_gpio;
            eeprom_access.send_byte = &send_byte_gpio;
            eeprom_access.recv_byte = &recv_byte_gpio;
            init_gpio(self);
            break;
        case BOARD_FLASH_EPP:
#if MESAFLASH_IO
            eeprom_access.set_cs_low = &set_cs_low_epp;
            eeprom_access.set_cs_high = &set_cs_high_epp;
            eeprom_access.prefix = &prefix_epp;
            eeprom_access.suffix = &suffix_epp;
            eeprom_access.send_byte = &send_byte_epp;
            eeprom_access.recv_byte = &recv_byte_epp;
#endif
            break;
        case BOARD_FLASH_REMOTE:
            break;
    }
    eeprom_access.read_page = &read_page;
    eeprom_access.write_page = &write_page;
    eeprom_access.erase_sector = &erase_sector;
    eeprom_access.start_programming = &local_start_programming;
};

void close_spi_access_local(llio_t *self) {
    board_t *board = self->board;

    switch (board->flash) {
        case BOARD_FLASH_NONE:
        case BOARD_FLASH_HM2:
        case BOARD_FLASH_IO:
        case BOARD_FLASH_EPP:
        case BOARD_FLASH_REMOTE:
            break;
        case BOARD_FLASH_GPIO:
            restore_gpio(self);
            break;
    }
}
