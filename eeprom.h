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

#ifndef __EEPROM_H
#define __EEPROM_H

#include "hostmot2.h"
#include "bitfile.h"

#define ID_EEPROM_1M 0x10
#define ID_EEPROM_2M 0x11
#define ID_EEPROM_4M 0x12
#define ID_EEPROM_8M 0x13
#define ID_EEPROM_16M 0x14
#define ID_EEPROM_32M 0x15

#define PAGE_SIZE   256
#define SECTOR_SIZE 65536
#define BOOT_BLOCK_SIZE 64

#define BOOT_ADDRESS     0x000000
#define XILINX_FALLBACK_ADDRESS 0x010000
#define EFINIX_FALLBACK_ADDRESS 0x000000

typedef struct {
    void (*set_cs_low)(llio_t *self);
    void (*set_cs_high)(llio_t *self);
    void (*prefix)(llio_t *self);
    void (*suffix)(llio_t *self);
    void (*send_byte)(llio_t *self, u8 byte);
    u8   (*recv_byte)(llio_t *self);
    void (*read_page)(llio_t *self, u32 addr, void *buff);
    void (*write_page)(llio_t *self, u32 addr, void *buff);
    void (*erase_sector)(llio_t *self, u32 addr);
    int  (*start_programming)(llio_t *self, u32 start_address, int fsize);
} spi_eeprom_dev_t;

extern u8 boot_block[BOOT_BLOCK_SIZE];

char *eeprom_get_flash_type(u8 flash_id);
u32 eeprom_calc_user_space(u8 flash_id);
void eeprom_prepare_boot_block(u8 flash_id);
int start_programming(llio_t *self, u32 start_address, int fsize);
int eeprom_write(llio_t *self, char *bitfile_name, u32 start_address, int fix_boot_flag, int sha256_check_flag);
int eeprom_verify(llio_t *self, char *bitfile_name, u32 start_address);
int flash_backup(llio_t *self, char *bitfile_name);
int flash_erase(llio_t *self);
int flash_restore(llio_t *self, char *bitfile_name);
void eeprom_init(llio_t *self);
void eeprom_cleanup(llio_t *self);

#endif
