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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "encoder_module.h"
#include "hostmot2.h"

double seconds_per_tsdiv_clock;

static void enable_encoder_pins(hostmot2_t *hm2) {
    int i;

    for (i = 0; i < HM2_MAX_PINS; i++) {
        if (hm2->pins[i].gtag == HM2_GTAG_NONE) {
            break;
        }
        if ((hm2->pins[i].sec_tag == HM2_GTAG_MUXED_ENCODER_SEL)) {
            hm2_set_pin_source(hm2, i, HM2_PIN_SOURCE_IS_SECONDARY);
            if (hm2->pins[i].sec_pin & HM2_PIN_OUTPUT) {
                hm2_set_pin_direction(hm2, i, HM2_PIN_DIR_IS_OUTPUT);
            }
        }
    }
}

// Return the physical ports to default
static void disable_encoder_pins(llio_t *llio) {
}

int encoder_init(encoder_module_t *enc, board_t *board, int instance, int delay) {
    hm2_module_desc_t *md = hm2_find_module(&(board->llio.hm2), HM2_GTAG_MUXED_ENCODER);
    u32 clock, control;

    if (md == NULL) {
        printf("No encoder module found.\n");
        return -1;
    }
    if (instance >= (md->instances - 1)) {
        printf("encoder instance number to high.\n");
        return -1;
    }

    memset(enc, 0, sizeof(encoder_module_t));
    enc->scale = 1.0;
    enc->board = board;
    enc->base_address = md->base_address;
    enc->instance = instance;
    enc->instance_stride = (md->strides & 0xF0) == 0 ? board->llio.hm2.idrom.instance_stride0 : board->llio.hm2.idrom.instance_stride1;

    enable_encoder_pins(&(enc->board->llio.hm2));

    if (md->clock_tag == HM2_CLOCK_LOW_TAG) {
        clock = (enc->board->llio.hm2.idrom.clock_low / 1e6 * delay) - 2;
        seconds_per_tsdiv_clock = (double)(clock + 2) / (double)enc->board->llio.hm2.idrom.clock_low;
    } else if (md->clock_tag == HM2_CLOCK_HIGH_TAG) {
        clock = (enc->board->llio.hm2.idrom.clock_high / 1e6 * delay) - 2;
        seconds_per_tsdiv_clock = (double)(clock + 2) / (double)enc->board->llio.hm2.idrom.clock_high;
    } 
    enc->board->llio.write(&(enc->board->llio), enc->base_address + HM2_MOD_OFFS_MUX_ENCODER_TSSDIV, &clock, sizeof(clock));
    enc->board->llio.read(&(enc->board->llio), enc->base_address + HM2_MOD_OFFS_MUX_ENCODER_LATCH_CCR + enc->instance*enc->instance_stride, &control, sizeof(control));
    control |= HM2_ENCODER_FILTER;
    control &= ~(HM2_ENCODER_QUADRATURE_ERROR);
    enc->board->llio.write(&(enc->board->llio), enc->base_address + HM2_MOD_OFFS_MUX_ENCODER_LATCH_CCR + enc->instance*enc->instance_stride, &control, sizeof(control));

    return 0;
}

int encoder_cleanup(encoder_module_t *enc) {
    disable_encoder_pins(&(enc->board->llio));

    return 0;
}

int encoder_read(encoder_module_t *enc) {
    board_t *board;
    u32 tsc, count, control;
    u16 reg_count, reg_time_stamp;
    int prev_raw_counts = enc->raw_counts, reg_count_diff, tsc_rollover = 0;
    int dT_clocks;
    double dT_seconds;
    int dS_counts;
    double dS_meters;

    if (enc == NULL) {
        return -1;
    }
    board = enc->board;

    board->llio.read(&(board->llio), enc->base_address + HM2_MOD_OFFS_MUX_ENCODER_TS_COUNT, &tsc, sizeof(tsc));
    board->llio.read(&(board->llio), enc->base_address + HM2_MOD_OFFS_MUX_ENCODER_COUNTER + enc->instance*enc->instance_stride, &count, sizeof(count));
    board->llio.read(&(board->llio), enc->base_address + HM2_MOD_OFFS_MUX_ENCODER_LATCH_CCR + enc->instance*enc->instance_stride, &control, sizeof(control));

    if (enc->scale == 0) {
        enc->scale = 1.0;
    }
    reg_count = count & 0xFFFF;
    reg_count_diff = (int)reg_count - enc->count;
    reg_time_stamp = count >> 16;
    if (reg_count_diff == 0) {
        enc->velocity = 0;
    } else {  
        if (reg_count_diff > 32768) reg_count_diff -= 65536;
        if (reg_count_diff < -32768) reg_count_diff += 65536;
        enc->raw_counts += reg_count_diff;

        if (reg_time_stamp < enc->time_stamp) {
        // tsc rollover!
             tsc_rollover++;
        }

        dT_clocks = (reg_time_stamp - enc->time_stamp) + (tsc_rollover << 16);
        dT_seconds = (double) dT_clocks * seconds_per_tsdiv_clock;
        dS_counts = enc->raw_counts - prev_raw_counts;
        dS_meters = (double) dS_counts / enc->scale;

        if (dT_clocks > 0) {
            enc->velocity = dS_meters / dT_seconds;
        }
    }

    enc->position = enc->raw_counts / enc->scale;
    enc->count = reg_count;
    enc->time_stamp = reg_time_stamp;
    enc->global_time_stamp = tsc;

    return 0;
}
