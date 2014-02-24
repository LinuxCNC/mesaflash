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

#ifndef __SPILBP_H
#define __SPILBP_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "common.h"

#define SPILBP_SENDRECV_DEBUG       0

#define SPILBP_MAX_PACKET_DATA_SIZE 0x7F

#define SPILBP_ADDR_AUTO_INC        0b0000100000000000
#define SPILBP_READ                 0b1010000000000000
#define SPILBP_WRITE                0b1011000000000000

typedef struct {
    u8 addr_hi;
    u8 addr_lo;
    u8 cmd_hi;
    u8 cmd_lo;
} spilbp_cmd_addr;

void spilbp_print_info();
void spilbp_init(board_access_t *access);
void spilbp_release();

#endif


