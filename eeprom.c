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

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <stdio.h>

#include "eeprom.h"
#include "eeprom_local.h"
#include "eeprom_remote.h"

u8 boot_block[BOOT_BLOCK_SIZE] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xAA, 0x99, 0x55, 0x66, 0x31, 0xE1,
    0xFF, 0xFF, 0x32, 0x61, 0x00, 0x00, 0x32, 0x81,
    0x0B, 0x08, 0x32, 0xA1, 0x00, 0x00, 0x32, 0xC1,
    0x0B, 0x01, 0x30, 0xA1, 0x00, 0x00, 0x33, 0x01,
    0x21, 0x00, 0x32, 0x01, 0x00, 0x1F, 0x30, 0xA1,
    0x00, 0x0E, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00
};

u8 page_buffer[PAGE_SIZE];
u8 file_buffer[SECTOR_SIZE];

char *eeprom_get_flash_type(u8 flash_id) {
    switch (flash_id) {
        case ID_EEPROM_1M:  return "1Mb";
        case ID_EEPROM_2M:  return "2Mb";
        case ID_EEPROM_4M:  return "4Mb";
        case ID_EEPROM_8M:  return "8Mb";
        case ID_EEPROM_16M: return "16Mb";
        default:            return "unknown";
    }
}

// modify MSB of boot block jmp address to user area
void eeprom_prepare_boot_block(u8 flash_id) {
    switch (flash_id) {
        case ID_EEPROM_1M:  boot_block[25] = 0x01; break;
        case ID_EEPROM_2M:  boot_block[25] = 0x02; break;
        case ID_EEPROM_4M:  boot_block[25] = 0x04; break;
        case ID_EEPROM_8M:  boot_block[25] = 0x08; break;
        case ID_EEPROM_16M: boot_block[25] = 0x10; break;
    }
}

u32 eeprom_calc_user_space(u8 flash_id) {
    switch (flash_id) {
        case ID_EEPROM_1M:  return 0x10000; break;
        case ID_EEPROM_2M:  return 0x20000; break;
        case ID_EEPROM_4M:  return 0x40000; break;
        case ID_EEPROM_8M:  return 0x80000; break;
        case ID_EEPROM_16M: return 0x100000; break;
        default: return 0x80000; break;
    }
}

void eeprom_init(llio_t *self) {
    board_t *board = self->private;

    switch (board->flash) {
        case BOARD_FLASH_NONE:
            break;
        case BOARD_FLASH_HM2:
        case BOARD_FLASH_IO:
        case BOARD_FLASH_GPIO:
            open_spi_access_local(self);
            break;
        case BOARD_FLASH_REMOTE:
            open_spi_access_remote(self);
            break;
    }
}

void eeprom_cleanup(llio_t *self) {
    board_t *board = self->private;

    switch (board->flash) {
        case BOARD_FLASH_NONE:
            break;
        case BOARD_FLASH_HM2:
        case BOARD_FLASH_IO:
        case BOARD_FLASH_GPIO:
            close_spi_access_local(self);
            break;
        case BOARD_FLASH_REMOTE:
            close_spi_access_remote(self);
            break;
    }
}
