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

#ifndef __SSERIAL_MODULE_H
#define __SSERIAL_MODULE_H

#include "boards.h"
#include "hostmot2.h"

struct opd_mode_1_7i77_struct {
    unsigned int analog0 : 13;
    unsigned int analog1 : 13;
    unsigned int analog2 : 13;
    unsigned int analog3 : 13;
    unsigned int analog4 : 13;
    unsigned int analog5 : 13;
    unsigned int analogena : 1;
    unsigned int spinena   : 1;
    unsigned int ignore1   : 16;
} __attribute__ ((__packed__));

typedef struct opd_mode_1_7i77_struct opd_mode_1_7i77_t;

typedef struct {
    board_t *board;
    int instance_stride;
    int interface_num;
    int channel_num;
    u16 base_address;
    u32 data;
    u32 cs;
    u32 interface0;
    u32 interface1;
    u32 interface2;
    sserial_interface_t interface;
    sserial_device_t device;
} sserial_module_t;

int sserial_init(sserial_module_t *ssmod, board_t *board, int interface_num, int channel_num, u32 remote_type);
int sserial_cleanup(sserial_module_t *ssmod);
void sserial_module_init(llio_t *llio);
int sserial_write(sserial_module_t *ssmod);

#endif
