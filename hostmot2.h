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

#ifndef __HOSTMOT2_H
#define __HOSTMOT2_H

#include "types.h"
#include "hostmot2_def.h"

typedef struct {
    int index;
    char name[64];
} sw_mode_t;

typedef struct {
    u32 unit;
    u16 sw_revision;
    u16 hw_revision;
    char name[4];
    sw_mode_t sw_modes[256];
    int sw_modes_cnt;
} sserial_device_t;

#define HM2_MAX_TAGS     255
#define MAX_BOB_NAMES 42
#define ANYIO_MAX_IOPORT_CONNECTORS 8

typedef struct llio_struct llio_t;

typedef struct {
    llio_t *llio;
    char config_name[8];
    hm2_idrom_desc_t idrom;
    hm2_module_desc_t modules[HM2_MAX_MODULES];
    hm2_pin_desc_t pins[HM2_MAX_PINS];
} hostmot2_t;

struct llio_struct {
    int (*read)(llio_t *self, u32 addr, void *buffer, int size);
    int (*write)(llio_t *self, u32 addr, void *buffer, int size);
    int (*write_flash)(llio_t *self, char *bitfile_name, u32 start_address, int fix_boot_flag, int sha256_check_flag);
    int (*verify_flash)(llio_t *self, char *bitfile_name, u32 start_address);
    int (*backup_flash)(llio_t *self, char *bitfile_name);
    int (*restore_flash)(llio_t *self, char *bitfile_name);
    int (*program_fpga)(llio_t *self, char *bitfile_name);
    int (*reset)(llio_t *self);
    int (*reload)(llio_t *self, int fallback_flag);
    int num_ioport_connectors;
    int pins_per_connector;
    const char *ioport_connector_name[ANYIO_MAX_IOPORT_CONNECTORS];
    u16 bob_hint[ANYIO_MAX_IOPORT_CONNECTORS];
    int num_leds;
    const char *fpga_part_number;
    char board_name[16];
    void *board;
    hostmot2_t hm2;
    sserial_interface_t ss_interface[HM2_SSERIAL_MAX_INTERFACES];
    sserial_device_t ss_device[HM2_SSERIAL_MAX_CHANNELS];
    int verbose;
};

typedef struct {
    u8 tag;
    char *name[32];
} pin_name_t;

typedef struct {
    u16   bobname;
    char *name[32];
} bob_pin_name_t;

typedef struct {
    char *name;
    u8 tag;
} mod_name_t;

void hm2_read_idrom(hostmot2_t *hm2);
hm2_module_desc_t *hm2_find_module(hostmot2_t *hm2, u8 gtag);
void hm2_print_pin_file(llio_t *llio, int xml_flag);
void hm2_print_pin_descriptors(llio_t *llio);
void hm2_print_localio_descriptors(llio_t *llio);
void hm2_enable_all_module_outputs(hostmot2_t *hm2);
void hm2_safe_io(hostmot2_t *hm2);
void hm2_set_pin_source(hostmot2_t *hm2, u32 pin_number, u8 source);
void hm2_set_pin_direction(hostmot2_t *hm2, u32 pin_number, u8 direction);
void sserial_module_init(llio_t *llio);

int hm2_find_bob_hint_by_name(const char *arg);
void hm2_print_bob_hint_names();

#endif

