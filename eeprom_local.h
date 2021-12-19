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

#ifndef __EEPROM_LOCAL_H
#define __EEPROM_LOCAL_H

#include "hostmot2.h"

#define SPI_DAV_MASK    0x04

// gpio access
#define XIO2001_SBAD_STAT_REG 0x00B2
#define XIO2001_GPIO_ADDR_REG 0x00B4
#define XIO2001_GPIO_DATA_REG 0x00B6

#define DATA_MASK 0x80
#define ADDR_MASK 0x800000

#define CMD_LEN  8
#define DATA_LEN 8
#define ADDR_LEN 24

// io access
#define IO_SPI_SREG_OFFSET    0x40
#define IO_SPI_CS_OFFSET      0x44

// epp access
#define EPP_SPI_CS_REG        0x7D
#define EPP_SPI_SREG_REG      0x7E

u8 read_flash_id(llio_t *self);
int local_write_flash(llio_t *self, char *bitfile_name, u32 start_address, int fix_boot_flag, int sha256_check_flag);
int local_verify_flash(llio_t *self, char *bitfile_name, u32 start_address);
int local_backup_flash(llio_t *self, char *bitfile_name);
int local_restore_flash(llio_t *self, char *bitfile_name);
void open_spi_access_local(llio_t *self);
void close_spi_access_local(llio_t *self);

#endif


