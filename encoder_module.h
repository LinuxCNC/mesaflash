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

#ifndef __ENCODER_MODULE_H
#define __ENCODER_MODULE_H

#include "boards.h"
#include "hostmot2.h"

typedef struct {
    u16 count;
    u16 time_stamp;
    u16 global_time_stamp;
    int raw_counts;
    double position;
    double velocity;
    double scale;
    int quad_error;
    board_t *board;
    u16 base_address;
    int instance;
    int instance_stride;
} encoder_module_t;

int encoder_init(encoder_module_t *enc, board_t *board, int instance, int delay);
int encoder_cleanup(encoder_module_t *enc);
int encoder_read(encoder_module_t *enc);

#endif
