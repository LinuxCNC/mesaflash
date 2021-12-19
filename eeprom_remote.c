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

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "types.h"
#include "eeprom.h"
#include "eeprom_remote.h"
#include "eth_boards.h"
#include "lbp16.h"
#include "bitfile.h"

extern u8 boot_block[BOOT_BLOCK_SIZE];
extern u8 page_buffer[PAGE_SIZE];
extern u8 file_buffer[SECTOR_SIZE];

extern spi_eeprom_dev_t eeprom_access;

// eeprom access functions

static void read_page(llio_t *self, u32 addr, void *buff) {
    (void)self;
    lbp16_cmd_addr_data32 write_addr_pck;
    lbp16_cmd_addr read_page_pck;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    LBP16_INIT_PACKET4(read_page_pck, CMD_READ_FPGA_FLASH_ADDR32(64), FLASH_DATA_REG);

    lbp16_send_packet_checked(&write_addr_pck, sizeof(write_addr_pck));
    lbp16_send_packet_checked(&read_page_pck, sizeof(read_page_pck));
    lbp16_recv_packet_checked(buff, PAGE_SIZE);
}

static void write_page(llio_t *self, u32 addr, void *buff) {
    (void)self;
    lbp16_cmd_addr_data32 write_addr_pck;
    lbp16_write_flash_page_packets write_page_pck;
    u32 ignored;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    lbp16_send_packet_checked(&write_addr_pck, sizeof(write_addr_pck));

    LBP16_INIT_PACKET6(write_page_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A03);
    LBP16_INIT_PACKET4(write_page_pck.fl_write_page_pck, CMD_WRITE_FPGA_FLASH_ADDR32(64), FLASH_DATA_REG);
    memcpy(&write_page_pck.fl_write_page_pck.page, buff, 256);
    lbp16_send_packet_checked(&write_page_pck, sizeof(write_page_pck));
    // packet read for board syncing
    lbp16_read(CMD_READ_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, &ignored, 4);
}

static void erase_sector(llio_t *self, u32 addr) {
    (void)self;
    lbp16_erase_flash_sector_packets sector_erase_pck;
    lbp16_cmd_addr_data32 write_addr_pck;
    u32 ignored;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    lbp16_send_packet_checked(&write_addr_pck, sizeof(write_addr_pck));

    LBP16_INIT_PACKET6(sector_erase_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A03);
    LBP16_INIT_PACKET8(sector_erase_pck.fl_erase_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_SEC_ERASE_REG, 0);
    lbp16_send_packet_checked(&sector_erase_pck, sizeof(sector_erase_pck));
    // packet read for board syncing
    lbp16_read(CMD_READ_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, &ignored, 4);
}

static int remote_start_programming(llio_t *self, u32 start_address, int fsize) {
    return start_programming(self, start_address, fsize);
}

// global functions

int remote_write_flash(llio_t *self, char *bitfile_name, u32 start_address, int fix_boot_flag, int sha256_check_flag) {
    return eeprom_write(self, bitfile_name, start_address, fix_boot_flag, sha256_check_flag);
}

int remote_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    return eeprom_verify(self, bitfile_name, start_address);
}

int remote_backup_flash(llio_t *self, char *bitfile_name) {
    return flash_backup(self, bitfile_name);
}

int remote_restore_flash(llio_t *self, char *bitfile_name) {
    return flash_restore(self, bitfile_name);
}

void open_spi_access_remote(llio_t *self) {
    (void)self;
    eeprom_access.read_page = &read_page;
    eeprom_access.write_page = &write_page;
    eeprom_access.erase_sector = &erase_sector;
    eeprom_access.start_programming = &remote_start_programming;
};

void close_spi_access_remote(llio_t *self) {
    (void)self;
};
