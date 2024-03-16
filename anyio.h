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

#ifndef __ANYIO_H
#define __ANYIO_H

#ifdef _WIN32
#include <windef.h>
#include "winio32/winio.h"
#endif
#include "boards.h"

int anyio_init(board_access_t *access);
void anyio_cleanup(board_access_t *access);
int anyio_find_dev(board_access_t *access);
board_t *anyio_get_dev(board_access_t *access, int board_number);
int anyio_dev_write_flash(board_t *board, char *bitfile_name, int fallback_flag, int fix_boot_flag, int sha256_check_flag);
int anyio_dev_verify_flash(board_t *board, char *bitfile_name, int fallback_flag);
int anyio_dev_backup_flash(board_t *board, char *bitfile_name);
int anyio_dev_restore_flash(board_t *board, char *bitfile_name);
int anyio_dev_program_fpga(board_t *board, char *bitfile_name);
int anyio_dev_set_remote_ip(board_t *board, char *lbp16_set_ip_addr);
int anyio_dev_set_led_mode(board_t *board, char *lbp16_set_led_mode);
int anyio_dev_reload(board_t *board, int fallback_flag);
int anyio_dev_reset(board_t *board);
void anyio_dev_print_hm2_info(board_t *board, int xml_flag);
void anyio_dev_print_pin_descriptors(board_t *board);
void anyio_dev_print_localio_descriptors(board_t *board);
void anyio_dev_enable_all_module_outputs(board_t *board);
void anyio_dev_safe_io(board_t *board);
void anyio_dev_print_sserial_info(board_t *board);
void anyio_bitfile_print_info(char *bitfile_name, int verbose_flag);
void anyio_print_supported_board_names();

#endif
