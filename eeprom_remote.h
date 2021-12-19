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

#ifndef __EEPROM_REMOTE_H
#define __EEPROM_REMOTE_H

#include "hostmot2.h"

int remote_write_flash(llio_t *self, char *bitfile_name, u32 start_address, int fix_boot_flag, int sha256_check_flag);
int remote_verify_flash(llio_t *self, char *bitfile_name, u32 start_address);
int remote_backup_flash(llio_t *self, char *bitfile_name);
int remote_restore_flash(llio_t *self, char *bitfile_name);
void open_spi_access_remote(llio_t *self);
void close_spi_access_remote(llio_t *self);

#endif


