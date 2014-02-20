#ifndef __EEPROM_H
#define __EEPROM_H

#include "anyio.h"
#include "hostmot2.h"

#define ID_EEPROM_1M 0x10
#define ID_EEPROM_2M 0x11
#define ID_EEPROM_4M 0x12
#define ID_EEPROM_8M 0x13
#define ID_EEPROM_16M 0x14

#define PAGE_SIZE   256
#define SECTOR_SIZE 65536
#define BOOT_BLOCK_SIZE 64

#define BOOT_ADDRESS     0x000000
#define FALLBACK_ADDRESS 0x010000

extern u8 boot_block[BOOT_BLOCK_SIZE];

char *eeprom_get_flash_type(u8 flash_id);
u32 eeprom_calc_user_space(u8 flash_id);
void eeprom_prepare_boot_block(u8 flash_id);
void eeprom_init(llio_t *self);
void eeprom_cleanup(llio_t *self);

#endif
