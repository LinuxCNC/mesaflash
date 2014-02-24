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
#include <sys/mman.h>
#include <sys/io.h>
#elif _WIN32
#include <windows.h>
#include "libpci/pci.h"
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "eeprom.h"
#include "eeprom_local.h"
#include "bitfile.h"
#include "pci_boards.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;
static int memfd = -1;
struct pci_access *pacc;
static u8 file_buffer[SECTOR_SIZE];

u16 setup_eeprom_5i20[256] = {
0x9030, // DEVICE ID
0x10B5, // VENDOR ID
0x0290, // PCI STATUS
0x0000, // PCI COMMAND
0x1180, // CLASS CODE (Changed to Data Acq to fool some BIOSs)
0x0000, // CLASS CODE / REV
0x3131, // SUBSYSTEM ID
0x10B5, // SUBSYSTEM VENDOR ID

0x0000, // MSB NEW CAPABILITY POINTER
0x0040, // LSB NEW CAPABILITY POINTER
0x0000, // RESERVED
0x0100, // INTERRUPT PIN
0x0000, // MSW OF POWER MANAGEMENT CAPABILITIES
0x0000, // LSW OF POWER MANAGEMANT CAPABILITIES
0x0000, // MSW OF POWER MANAGEMENT DATA / PMCSR BRIDGE SUPPORT EXTENSION
0x0000, // LSW OF POWER MANAGEMENT CONTROL STATUS

0x0000, // MSW OF HOT SWAP CONTROL / STATUS
0x0000, // LSW OF HOTSWAP NEXT CAPABILITY POINTER / HOT SWAP CONTROL
0x0000, // PCI VITAL PRODUCT DATA ADDRESS
0x0000, // PCI VITAL PRODUCT NEXT CAPABILITY POINTER / PCI VITAL PRODUCT DATA CONTROL
0x0FFF, // MSW OF LOCAL ADDRESS SPACE 0 RANGE
0xFF01, // LSW OF LOCAL ADDRESS SPACE 0 RANGE
0x0FFF, // MSW OF LOCAL ADDRESS SPACE 1 RANGE
0xFF01, // LSW OF LOCAL ADDRESS SPACE 1 RANGE

0x0FFF, // MSW OF LOCAL ADDRESS SPACE 2 RANGE
0x0000, // LSW OF LOCAL ADDRESS SPACE 2 RANGE
0x0FFF, // MSW OF LOCAL ADDRESS SPACE 3 RANGE
0x0000, // LSW OF LOCAL ADDRESS SPACE 3 RANGE
0x0000, // MSW OF EXPANSION ROM RANGE
0x0000, // LSW OF EXPANSION ROM RANGE
0x0000, // MSW OF LOCAL ADDRESS SPACE 0 LOCAL BASE ADDRESS (REMAP)
0x0001, // LSW OF LOCAL ADDRESS SPACE 0 LOCAL BASE ADDRESS (REMAP)

0x0000, // MSW OF LOCAL ADDRESS SPACE 1 LOCAL BASE ADDRESS (REMAP)
0x0001, // LSW OF LOCAL ADDRESS SPACE 1 LOCAL BASE ADDRESS (REMAP)
0x0000, // MSW OF LOCAL ADDRESS SPACE 2 LOCAL BASE ADDRESS (REMAP)
0x0001, // LSW OF LOCAL ADDRESS SPACE 2 LOCAL BASE ADDRESS (REMAP)
0x0000, // MSW OF LOCAL ADDRESS SPACE 3 LOCAL BASE ADDRESS (REMAP)
0x0001, // LSW OF LOCAL ADDRESS SPACE 3 LOCAL BASE ADDRESS (REMAP)
0x0000, // MSW OF EXPANSION ROM LOCAL BASE ADDRESS (REMAP)
0x0000, // LSW OF EXPANSION ROM LOCAL BASE ADDRESS (REMAP)

0x0040, // MSW OF LOCAL ADDRESS SPACE 0 BUS DESCRIPTOR
0x0002, // LSW OF LOCAL ADDRESS SPACE 0 BUS DESCRIPTOR
0x0080, // MSW OF LOCAL ADDRESS SPACE 1 BUS DESCRIPTOR
0x0002, // LSW OF LOCAL ADDRESS SPACE 1 BUS DESCRIPTOR
0x0040, // MSW OF LOCAL ADDRESS SPACE 2 BUS DESCRIPTOR
0x0002, // LSW OF LOCAL ADDRESS SPACE 2 BUS DESCRIPTOR
0x0080, // MSW OF LOCAL ADDRESS SPACE 3 BUS DESCRIPTOR
0x0002, // LSW OF LOCAL ADDRESS SPACE 3 BUS DESCRIPTOR

0x0000, // MSW OF EXPANSION ROM BUS DESCRIPTOR
0x0002, // LSW OF EXPANSION ROM BUS DESCRIPTOR
0x0000, // MSW OF CHIP SELECT 0 BASE ADDRESS
0x0000, // LSW OF CHIP SELECT 0 BASE ADDRESS
0x0000, // MSW OF CHIP SELECT 1 BASE ADDRESS
0x0000, // LSW OF CHIP SELECT 1 BASE ADDRESS
0x0000, // MSW OF CHIP SELECT 2 BASE ADDRESS
0x0000, // LSW OF CHIP SELECT 2 BASE ADDRESS

0x0000, // MSW OF CHIP SELECT 3 BASE ADDRESS
0x0000, // LSW OF CHIP SELECT 3 BASE ADDRESS
0x0000, // SERIAL EEPROM WRITE PROTECTED ADDRESS BOUNDARY
0x0041, // LSW OF INTERRUPT CONTROL / STATUS
0x0878, // MSW OF PCI TARGET RESPONSE, SERIAL EEPROM, AND INITIALIZATION CONTROL
0x0000, // LSW OF PCI TARGET RESPONSE, SERIAL EEPROM, AND INITIALIZATION CONTROL
0x024B, // MSW OF GENERAL PURPOSE I/O CONTROL
0x009B, // LSW OF GENERAL PURPOSE I/O CONTROL
};

u16 setup_eeprom_3x20_10[256] = {
0x9056, // DEVICE ID  					PCIIDR[31:16]
0x10B5, // VENDOR ID 					PCIIDR[15:0]
0x0680, // CLASS CODE					PCICCR[23:8]
0x0000, // CLASS CODE/REVISION				PCICCR[7:0]/PCIREV[7:0]
0x0000, // MAXIMUM LATENCY/MINIMUM GRANT		PCIMLR[7:0]/PCIMGR[7:0]
0x0100, // INTERRUPT PIN/INTERRUPT PIN ROUTING		PCIIPR[7:0]/PCIIL[7:0]
0x0000, // MSW OF MAILBOX0				MBOX0[31:16]
0x0000, // LSW OF MAILBOX0				MBOX0[15:0]

0x0000, // MSW OF MAILBOX1				MBOX1[31:16]
0x0000, // LSW OF MAILBOX1				MBOX1[15:0]
0xFFFF, // MSW OF PCI-LOCAL RANGE0			LAS0RR[31:16]
0xFF01, // LSW OF PCI-LOCAL RANGE0			LAS0RR[15:0]
0x0000, // MSW OF PCI-LOCAL REMAP0			LAS0BA[31:16]
0x0001, // LSW OF PCI-LOCAL REMAP0			LAS0BA[15:0]
0x0200, // MSW OF DMA ARBITRATION REGISTER		MARBR[31:16]
0x0000, // LSW OF DMA ARBITRATION REGISTER		MARBR[15:0]

0x0030, // MSW OF SERIAL EEPROM WP ADDRESS		PROT_AREA[15:0]
0x0500, // LSW OF MISC CONT REG/LSW OF BIG/LITTLE REG	LMISC[7:0]/BIGEND[7:0]
0xFFFF, // MSW OF PCI EXPANSION ROM RANGE		EROMRR[31:16]
0x0000, // LSW OF PCI EXPANSION ROM RANGE		EROMRR[15:0]
0x0000, // MSW OF PCI EXPANSION ROM REMAP		EROMBA[31:16]
0x0000, // LSW OF PCI EXPANSION ROM REMAP		EROMBA[15:0]
0x4243, // MSW OF BUS DESCRIPTORS FOR LOCAL SPACE 0	LBRD0[31:16]
0x0043, // LSW OF BUS DESCRIPTORS FOR LOCAL SPACE 0	LBRD0[15:0]

0x0000, // MSW OF PCI INITIATOR - PCI RANGE		DMRR[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI RANGE	        DMRR[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI LOCAL BASE ADD	DMLBAM[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI LOCAL BASE ADD	DMLBAM[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI LBA I/O CONF	DMLBA[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI LBA I/O CONF	DMLBA[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI BASE ADDRESS	DMPBAM[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI BASE ADDRESS	DMPBAM[15:0]

0x0000, // MSW OF PCI INITIATOR PCI CONF ADD		DMCFGA[31:16]
0x0000, // LSW OF PCI INITIATOR PCI CONF ADD		DMCFGA[15:0]
0x3427, // SUBSYSTEM ID					PCISID[15:0]
0x10B5, // SUBSYSTEM VENDOR ID				PCISVID[15:0]
0xFFFF, // MSW OF PCI-LOCAL RANGE1			LAS1RR[31:16]
0x0000, // LSW OF PCI-LOCAL RANGE1			LAS1RR[15:0]
0x0000, // MSW OF PCI-LOCAL REMAP1			LAS1BA[31:16]
0x0001, // LSW OF PCI-LOCAL REMAP1			LAS1BA[15:0]

0x0000, // MSW OF BUS DESCRIPTORS FOR LOCAL SPACE 1	LBRD1[31:16]
0x0043, // LSW OF BUS DESCRIPTORS FOR LOCAL SPACE 1	LBRD1[15:0]
0x0000, // MSW OF HOT-SWAP CONTROL			RESERVED
0x4C06, // LSW OF HS NEXT-CAP POINTER/HS CONT	HS_NEXT/HS_CNTL[7:0]
};

u16 setup_eeprom_3x20_15[256] = {
0x9056, // DEVICE ID  					PCIIDR[31:16]
0x10B5, // VENDOR ID 					PCIIDR[15:0]
0x0680, // CLASS CODE					PCICCR[23:8]
0x0000, // CLASS CODE/REVISION				PCICCR[7:0]/PCIREV[7:0]
0x0000, // MAXIMUM LATENCY/MINIMUM GRANT		PCIMLR[7:0]/PCIMGR[7:0]
0x0100, // INTERRUPT PIN/INTERRUPT PIN ROUTING		PCIIPR[7:0]/PCIIL[7:0]
0x0000, // MSW OF MAILBOX0				MBOX0[31:16]
0x0000, // LSW OF MAILBOX0				MBOX0[15:0]

0x0000, // MSW OF MAILBOX1				MBOX1[31:16]
0x0000, // LSW OF MAILBOX1				MBOX1[15:0]
0xFFFF, // MSW OF PCI-LOCAL RANGE0			LAS0RR[31:16]
0xFF01, // LSW OF PCI-LOCAL RANGE0			LAS0RR[15:0]
0x0000, // MSW OF PCI-LOCAL REMAP0			LAS0BA[31:16]
0x0001, // LSW OF PCI-LOCAL REMAP0			LAS0BA[15:0]
0x0200, // MSW OF DMA ARBITRATION REGISTER		MARBR[31:16]
0x0000, // LSW OF DMA ARBITRATION REGISTER		MARBR[15:0]

0x0030, // MSW OF SERIAL EEPROM WP ADDRESS		PROT_AREA[15:0]
0x0500, // LSW OF MISC CONT REG/LSW OF BIG/LITTLE REG	LMISC[7:0]/BIGEND[7:0]
0xFFFF, // MSW OF PCI EXPANSION ROM RANGE		EROMRR[31:16]
0x0000, // LSW OF PCI EXPANSION ROM RANGE		EROMRR[15:0]
0x0000, // MSW OF PCI EXPANSION ROM REMAP		EROMBA[31:16]
0x0000, // LSW OF PCI EXPANSION ROM REMAP		EROMBA[15:0]
0x4243, // MSW OF BUS DESCRIPTORS FOR LOCAL SPACE 0	LBRD0[31:16]
0x0043, // LSW OF BUS DESCRIPTORS FOR LOCAL SPACE 0	LBRD0[15:0]

0x0000, // MSW OF PCI INITIATOR - PCI RANGE		DMRR[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI RANGE	        DMRR[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI LOCAL BASE ADD	DMLBAM[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI LOCAL BASE ADD	DMLBAM[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI LBA I/O CONF	DMLBA[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI LBA I/O CONF	DMLBA[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI BASE ADDRESS	DMPBAM[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI BASE ADDRESS	DMPBAM[15:0]

0x0000, // MSW OF PCI INITIATOR PCI CONF ADD		DMCFGA[31:16]
0x0000, // LSW OF PCI INITIATOR PCI CONF ADD		DMCFGA[15:0]
0x3428, // SUBSYSTEM ID					PCISID[15:0]
0x10B5, // SUBSYSTEM VENDOR ID				PCISVID[15:0]
0xFFFF, // MSW OF PCI-LOCAL RANGE1			LAS1RR[31:16]
0x0000, // LSW OF PCI-LOCAL RANGE1			LAS1RR[15:0]
0x0000, // MSW OF PCI-LOCAL REMAP1			LAS1BA[31:16]
0x0001, // LSW OF PCI-LOCAL REMAP1			LAS1BA[15:0]

0x0000, // MSW OF BUS DESCRIPTORS FOR LOCAL SPACE 1	LBRD1[31:16]
0x0043, // LSW OF BUS DESCRIPTORS FOR LOCAL SPACE 1	LBRD1[15:0]
0x0000, // MSW OF HOT-SWAP CONTROL			RESERVED
0x4C06, // LSW OF HS NEXT-CAP POINTER/HS CONT	HS_NEXT/HS_CNTL[7:0]
};

u16 setup_eeprom_3x20_20[256] = {
0x9056, // DEVICE ID  					PCIIDR[31:16]
0x10B5, // VENDOR ID 					PCIIDR[15:0]
0x0680, // CLASS CODE					PCICCR[23:8]
0x0000, // CLASS CODE/REVISION				PCICCR[7:0]/PCIREV[7:0]
0x0000, // MAXIMUM LATENCY/MINIMUM GRANT		PCIMLR[7:0]/PCIMGR[7:0]
0x0100, // INTERRUPT PIN/INTERRUPT PIN ROUTING		PCIIPR[7:0]/PCIIL[7:0]
0x0000, // MSW OF MAILBOX0				MBOX0[31:16]
0x0000, // LSW OF MAILBOX0				MBOX0[15:0]

0x0000, // MSW OF MAILBOX1				MBOX1[31:16]
0x0000, // LSW OF MAILBOX1				MBOX1[15:0]
0xFFFF, // MSW OF PCI-LOCAL RANGE0			LAS0RR[31:16]
0xFF01, // LSW OF PCI-LOCAL RANGE0			LAS0RR[15:0]
0x0000, // MSW OF PCI-LOCAL REMAP0			LAS0BA[31:16]
0x0001, // LSW OF PCI-LOCAL REMAP0			LAS0BA[15:0]
0x0200, // MSW OF DMA ARBITRATION REGISTER		MARBR[31:16]
0x0000, // LSW OF DMA ARBITRATION REGISTER		MARBR[15:0]

0x0030, // MSW OF SERIAL EEPROM WP ADDRESS		PROT_AREA[15:0]
0x0500, // LSW OF MISC CONT REG/LSW OF BIG/LITTLE REG	LMISC[7:0]/BIGEND[7:0]
0xFFFF, // MSW OF PCI EXPANSION ROM RANGE		EROMRR[31:16]
0x0000, // LSW OF PCI EXPANSION ROM RANGE		EROMRR[15:0]
0x0000, // MSW OF PCI EXPANSION ROM REMAP		EROMBA[31:16]
0x0000, // LSW OF PCI EXPANSION ROM REMAP		EROMBA[15:0]
0x4243, // MSW OF BUS DESCRIPTORS FOR LOCAL SPACE 0	LBRD0[31:16]
0x0043, // LSW OF BUS DESCRIPTORS FOR LOCAL SPACE 0	LBRD0[15:0]

0x0000, // MSW OF PCI INITIATOR - PCI RANGE		DMRR[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI RANGE	        DMRR[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI LOCAL BASE ADD	DMLBAM[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI LOCAL BASE ADD	DMLBAM[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI LBA I/O CONF	DMLBA[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI LBA I/O CONF	DMLBA[15:0]
0x0000, // MSW OF PCI INITIATOR - PCI BASE ADDRESS	DMPBAM[31:16]
0x0000, // LSW OF PCI INITIATOR - PCI BASE ADDRESS	DMPBAM[15:0]

0x0000, // MSW OF PCI INITIATOR PCI CONF ADD		DMCFGA[31:16]
0x0000, // LSW OF PCI INITIATOR PCI CONF ADD		DMCFGA[15:0]
0x3429, // SUBSYSTEM ID					PCISID[15:0]
0x10B5, // SUBSYSTEM VENDOR ID				PCISVID[15:0]
0xFFFF, // MSW OF PCI-LOCAL RANGE1			LAS1RR[31:16]
0x0000, // LSW OF PCI-LOCAL RANGE1			LAS1RR[15:0]
0x0000, // MSW OF PCI-LOCAL REMAP1			LAS1BA[31:16]
0x0001, // LSW OF PCI-LOCAL REMAP1			LAS1BA[15:0]

0x0000, // MSW OF BUS DESCRIPTORS FOR LOCAL SPACE 1	LBRD1[31:16]
0x0043, // LSW OF BUS DESCRIPTORS FOR LOCAL SPACE 1	LBRD1[15:0]
0x0000, // MSW OF HOT-SWAP CONTROL			RESERVED
0x4C06, // LSW OF HS NEXT-CAP POINTER/HS CONT	HS_NEXT/HS_CNTL[7:0]
};

static void plx9030_SetCSHigh(board_t *board) {
    u16 data = inw(board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);

    data = data | PLX9030_EECS_MASK;
    outw(data, board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);
    sleep_ns(4000);
}

static void plx9030_SetCSLow(board_t *board) {
    u16 data = inw(board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);

    data = data & (~PLX9030_EECS_MASK);
    outw(data, board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);
    sleep_ns(4000);
}

static void plx9030_SetDinHigh(board_t *board) {
    u16 data = inw(board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);

    data = data | PLX9030_EEDI_MASK;
    outw(data, board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);
    sleep_ns(4000);
}

static void plx9030_SetDinLow(board_t *board) {
    u16 data = inw(board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);

    data = data & (~PLX9030_EEDI_MASK);
    outw(data, board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);
    sleep_ns(4000);
}

static void plx9030_SetClockHigh(board_t *board) {
    u16 data = inw(board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);

    data = data | PLX9030_EECLK_MASK;
    outw(data, board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);
    sleep_ns(4000);
}

static void plx9030_SetClockLow(board_t *board) {
    u16 data = inw(board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);

    data = data & (~PLX9030_EECLK_MASK);
    outw(data, board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);
    sleep_ns(4000);
}

static int plx9030_DataHighQ(board_t *board) {
    u16 data = inw(board->ctrl_base_addr + PLX9030_CTRL_INIT_OFFSET);

    sleep_ns(4000);
    if ((data & PLX9030_EEDO_MASK) != 0)
        return 1;
    else
        return 0;
}

static u16 plx9030_read_eeprom_word(board_t *board, u8 reg) {
    u8 bit;
    u16 mask;
    u16 tdata;

    plx9030_SetCSLow(board);
    plx9030_SetDinLow(board);
    plx9030_SetClockLow(board);
    plx9030_SetCSHigh(board);
    // send command first
    mask = EEPROM_93C66_CMD_MASK;
    for (bit = 0; bit < EEPROM_93C66_CMD_LEN; bit++) {
        if ((mask & EEPROM_93C66_CMD_READ) == 0)
            plx9030_SetDinLow(board);
        else
            plx9030_SetDinHigh(board);
        mask = mask >> 1;
        plx9030_SetClockLow(board);
        plx9030_SetClockHigh(board);
    }
    // then send address
    mask = EEPROM_93C66_ADDR_MASK;
    for (bit = 0; bit < EEPROM_93C66_ADDR_LEN; bit++) {
        if ((mask & reg) == 0)
            plx9030_SetDinLow(board);
        else
            plx9030_SetDinHigh(board);
        mask = mask >> 1;
        plx9030_SetClockLow(board);
        plx9030_SetClockHigh(board);
    }
    // read dummy 0 bit, if zero assume ok
    if (plx9030_DataHighQ(board) == 1)
        return 0;
    mask = EEPROM_93C66_DATA_MASK;
    tdata = 0;
    for (bit = 0; bit < EEPROM_93C66_DATA_LEN; bit++) {
        plx9030_SetClockLow(board);
        plx9030_SetClockHigh(board);
        if (plx9030_DataHighQ(board) == 1)
            tdata = tdata | mask;
        mask = mask >> 1;
    }
    plx9030_SetCSLow(board);
    plx9030_SetDinLow(board);
    plx9030_SetClockLow(board);
    return tdata;
}

void pci_plx9030_bridge_eeprom_setup_read(board_t *board) {
    int i;
    char *bridge_name = "Unknown";

    if (board->dev->device_id == DEVICEID_PLX9030)
        bridge_name = "PLX9030";
    else if (board->dev->device_id == DEVICEID_PLX9054)
        bridge_name = "PLX9054";
    else if (board->dev->device_id == DEVICEID_PLX9056)
        bridge_name = "PLX9056";

    printf("%s PCI bridge setup EEPROM:\n", bridge_name);
    for (i = 0; i < EEPROM_93C66_SIZE; i++) {
        if ((i > 0) && ((i % 16) == 0))
            printf("\n");
        if ((i % 16) == 0)
            printf("  %02X: ", i);
        printf("%04X ", plx9030_read_eeprom_word(board, i));
    }
    printf("\n");
}

static u32 plx9056_read_eeprom_dword(board_t *board, u16 reg) {
    u32 ret;
    u16 data;
    int i;

    pci_write_word(board->dev, PLX9056_VPD_ADDR, reg & ~PLX9056_VPD_FMASK);
    for (i = 0; i < 100000; i++) {
        data = pci_read_word(board->dev, PLX9056_VPD_ADDR);
        if ((data & PLX9056_VPD_FMASK) != 0)
            break;
    }
    if (i == 100000)
        return -1;
    ret = pci_read_word(board->dev, PLX9056_VPD_DATA);
    ret |= pci_read_word(board->dev, PLX9056_VPD_DATA + 2) << 16;
    return ret;
}

static u16 plx9056_read_eeprom_word(board_t *board, u16 reg) {
    u16 addr = (reg << 1) & 0xFFFC;
    u16 ret;
    u32 data = plx9056_read_eeprom_dword(board, addr);

    if ((reg & 0x01) == 0)
        ret = (data >> 16) & 0xFFFF;
    else
        ret = data & 0xFFFF;
    return ret;
}

static int plx9030_program_fpga(llio_t *self, char *bitfile_name) {
    board_t *board = self->private;
    int bindex, bytesread;
    u32 status, control;
    char part_name[32];
    struct stat file_stat;
    FILE *fp;

    if (stat(bitfile_name, &file_stat) != 0) {
        printf("Can't find file %s\n", bitfile_name);
        return -1;
    }
    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return -1;
    }
    if (print_bitfile_header(fp, (char*) &part_name) == -1) {
        fclose(fp);
        return -1;
    }
    // set /WRITE low for data transfer, and turn on LED
    status = inl(board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);
    control = status & ~PLX9030_WRITE_MASK & ~PLX9030_LED_MASK;
    outl(control, board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);

    printf("Programming FPGA...\n");
    printf("  |");
    fflush(stdout);
    // program the FPGA
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            outb(bitfile_reverse_bits(file_buffer[bindex]), board->data_base_addr);
            bindex++;
        }
        printf("W");
        fflush(stdout);
    }

    printf("\n");
    fclose(fp);

    // all bytes transferred, make sure FPGA is all set up now
    status = inl(board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);
    if (!(status & PLX9030_INIT_MASK)) {
    // /INIT goes low on CRC error
        printf("FPGA asserted /INIT: CRC error\n");
        goto fail;
    }
    if (!(status & PLX9030_DONE_MASK)) {
        printf("FPGA did not assert DONE\n");
        goto fail;
    }

    // turn off write enable and LED
    control = status | PLX9030_WRITE_MASK | PLX9030_LED_MASK;
    outl(control, board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);

    printf("Board FPGA programmed successfully\n");
    return 0;


fail:
    // set /PROGRAM low (reset device), /WRITE high and LED off
    status = inl(board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);
    control = status & ~PLX9030_PROGRAM_MASK;
    control |= PLX9030_WRITE_MASK | PLX9030_LED_MASK;
    outl(control, board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);
    return -EIO;
}

static int plx9030_reset(llio_t *self) {
    board_t *board = self->private;
    u32 status;
    u32 control;

    status = inl(board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);

    // set /PROGRAM bit low to reset the FPGA
    control = status & ~PLX9030_PROGRAM_MASK;

    // set /WRITE and /LED high (idle state)
    control |= PLX9030_WRITE_MASK | PLX9030_LED_MASK;

    // and write it back
    outl(control, board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);

    // verify that /INIT and DONE went low
    status = inl(board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);
    if (status & (PLX9030_DONE_MASK | PLX9030_INIT_MASK)) {
        printf("FPGA did not reset: /INIT = %d, DONE = %d\n",
            (status & PLX9030_INIT_MASK ? 1 : 0),
            (status & PLX9030_DONE_MASK ? 1 : 0)
        );
        return -EIO;
    }

    // set /PROGRAM high, let FPGA come out of reset
    control = status | PLX9030_PROGRAM_MASK;
    outl(control, board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);

    // wait for /INIT to go high when it finishes clearing memory
    // This should take no more than 100uS.  If we assume each PCI read
    // takes 30nS (one PCI clock), that is 3300 reads.  Reads actually
    // take several clocks, but even at a microsecond each, 3.3mS is not
    // an excessive timeout value
    {
        int count = 3300;

        do {
            status = inl(board->ctrl_base_addr + PLX9030_CTRL_STAT_OFFSET);
            if (status & PLX9030_INIT_MASK) break;
        } while (count-- > 0);

        if (count == 0) {
            printf("FPGA did not come out of /INIT\n");
            return -EIO;
        }
    }

    return 0;
}

static void plx9030_fixup_LASxBRD_READY(llio_t *self) {
    board_t *board = self->private;
    int offsets[] = {PLX9030_LAS0BRD_OFFSET, PLX9030_LAS1BRD_OFFSET, PLX9030_LAS2BRD_OFFSET, PLX9030_LAS3BRD_OFFSET};
    int i;

    for (i = 0; i < 4; i ++) {
        u32 val;
        int addr = board->ctrl_base_addr + offsets[i];

        val = inl(addr);
        if (!(val & PLX9030_LASxBRD_READY)) {
            printf("LAS%dBRD #READY is off, enabling now\n", i);
            val |= PLX9030_LASxBRD_READY;
            outl(val, addr);
        }
    }
}

static int plx905x_program_fpga(llio_t *self, char *bitfile_name) {
    board_t *board = self->private;
    int bindex, bytesread, i;
    u32 status;
    char part_name[32];
    struct stat file_stat;
    FILE *fp;

    if (stat(bitfile_name, &file_stat) != 0) {
        printf("Can't find file %s\n", bitfile_name);
        return -1;
    }
    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return -1;
    }
    if (print_bitfile_header(fp, (char*) &part_name) == -1) {
        fclose(fp);
        return -1;
    }
    printf("Programming FPGA...\n");
    printf("  |");
    fflush(stdout);
    // program the FPGA
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            outb(bitfile_reverse_bits(file_buffer[bindex]), board->data_base_addr);
            bindex++;
        }
        printf("W");
        fflush(stdout);
    }

    printf("\n");
    fclose(fp);


    // all bytes transferred, make sure FPGA is all set up now
    for (i = 0; i < PLX905X_DONE_WAIT; i++) {
        status = inl(board->ctrl_base_addr + PLX905X_CTRL_STAT_OFFSET);
        if (status & PLX905X_DONE_MASK) break;
    }
    if (i >= PLX905X_DONE_WAIT) {
        printf("Error: Not /DONE; programming not completed.\n");
        return -EIO;
    }

    printf("Board FPGA programmed successfully\n");
    return 0;
}

static int plx905x_reset(llio_t *self) {
    board_t *board = self->private;
    int i;
    u32 status, control;

    // set GPIO bits to GPIO function
    status = inl(board->ctrl_base_addr + PLX905X_CTRL_STAT_OFFSET);
    control = status | PLX905X_DONE_ENABLE | PLX905X_PROG_ENABLE;
    outl(control, board->ctrl_base_addr + PLX905X_CTRL_STAT_OFFSET);

    // Turn off /PROGRAM bit and insure that DONE isn't asserted
    outl(control & ~PLX905X_PROGRAM_MASK, board->ctrl_base_addr + PLX905X_CTRL_STAT_OFFSET);

    status = inl(board->ctrl_base_addr + PLX905X_CTRL_STAT_OFFSET);
    if (status & PLX905X_DONE_MASK) {
        // Note that if we see DONE at the start of programming, it's most
        // likely due to an attempt to access the FPGA at the wrong I/O
        // location.
        printf("/DONE status bit indicates busy at start of programming\n");
        return -EIO;
    }

    // turn on /PROGRAM output bit
    outl(control | PLX905X_PROGRAM_MASK, board->ctrl_base_addr + PLX905X_CTRL_STAT_OFFSET);

    // Delay for at least 100 uS. to allow the FPGA to finish its reset
    // sequencing.  3300 reads is at least 100 us, could be as long as a
    // few ms
    for (i = 0; i < 3300; i++) {
        status = inl(board->ctrl_base_addr + PLX905X_CTRL_STAT_OFFSET);
    }

    return 0;
}

#ifdef _WIN32
static void pci_fix_bar_lengths(struct pci_dev *dev) {
    int i;

    for (i = 0; i < 6; i++) {
        u32 saved_bar, size;

        if (dev->base_addr == 0)
            continue;

        saved_bar = pci_read_long(dev, PCI_BASE_ADDRESS_0 + i*4);
        pci_write_long(dev, PCI_BASE_ADDRESS_0 + i*4, 0xFFFFFFFF);
        size = pci_read_long(dev, PCI_BASE_ADDRESS_0 + i*4);
        if (size & PCI_BASE_ADDRESS_SPACE_IO)
            size = ~(size & PCI_BASE_ADDRESS_IO_MASK) & 0xFF;
        else
            size = ~(size & PCI_BASE_ADDRESS_MEM_MASK);
        pci_write_long(dev, PCI_BASE_ADDRESS_0 + i*4, saved_bar);

        dev->size[i] = size + 1;
    }
}
#endif

int pci_read(llio_t *self, u32 addr, void *buffer, int size) {
    board_t *board = self->private;

    memcpy(buffer, (board->base + addr), size);
//    printf("READ %X, (%X + %X), %d\n", buffer, board->base, addr, size);
    return 0;
}

int pci_write(llio_t *self, u32 addr, void *buffer, int size) {
    board_t *board = self->private;

    memcpy((board->base + addr), buffer, size);
    return 0;
}

static int pci_board_open(board_t *board) {
    if (board->mem_base != 0) {
#ifdef __linux__
        board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, board->mem_base);
#elif _WIN32
        pci_fix_bar_lengths(dev);
        board->base = map_memory(board->mem_base, board->len, &(board->mem_handle));
#endif
    }

    if (board->flash != BOARD_FLASH_NONE) {
        eeprom_init(&(board->llio));
        board->flash_id = read_flash_id(&(board->llio));
        if (board->fallback_support == 1)
            board->flash_start_address = eeprom_calc_user_space(board->flash_id);
        else
            board->flash_start_address = 0;

        eeprom_prepare_boot_block(board->flash_id);
    }
    return 0;
}

static int pci_board_close(board_t *board) {
    if (board->base) {
#ifdef __linux__
        munmap(board->base, board->len);
#elif _WIN32
        unmap_memory(&(board->mem_handle));
#endif
    }
    eeprom_cleanup(&(board->llio));

    return 0;
}

int pci_boards_init(board_access_t *access) {
    int eno;

    pacc = pci_alloc();
    pci_init(pacc);            // inicjowanie biblioteki libpci

#ifdef __linux__
    if (seteuid(0) != 0) {
        printf("%s need root privilges (or setuid root)\n", __func__);
        return -1;
    }
    memfd = open("/dev/mem", O_RDWR);
    eno = errno;
    seteuid(getuid());
    if (memfd < 0) {
        printf("%s can't open /dev/mem: %s", __func__, strerror(eno));
        return -1;
    }
    iopl(3);
#elif _WIN32
    init_io_library();
#endif
    return 0;
}

void pci_boards_cleanup(board_access_t *access) {
#ifdef __linux__
    close(memfd);
#elif _WIN32
    release_io_library();
#endif
    pci_cleanup(pacc);
}

void pci_boards_scan(board_access_t *access) {
    struct pci_dev *dev;
    board_t *board;

    pci_scan_bus(pacc);

    for (dev = pacc->devices; dev != NULL; dev = dev->next)
        // first run - fill data struct
        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_IRQ | PCI_FILL_BASES | PCI_FILL_ROM_BASE | PCI_FILL_SIZES | PCI_FILL_CLASS);

    if (access->recover == 1) {
        for (dev = pacc->devices; dev != NULL; dev = dev->next) {
            board = &boards[boards_count];
            if ((dev->vendor_id == VENDORID_XIO2001) && (dev->device_id == DEVICEID_XIO2001)) {
                board->type = BOARD_PCI;
                strncpy(board->llio.board_name, "6I25 (RECOVER)", 14);
                board->llio.num_ioport_connectors = 2;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "P3";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.fpga_part_number = "6slx9pq144";
                board->llio.num_leds = 2;
                board->llio.write_flash = &local_write_flash;
                board->llio.verify_flash = &local_verify_flash;
                board->llio.private = board;

                board->open = &pci_board_open;
                board->close = &pci_board_close;
                board->print_info = &pci_print_info;
                board->mem_base = 0;
                board->dev = dev;
                board->flash = BOARD_FLASH_GPIO;
                board->fallback_support = 1;
                board->llio.verbose = access->verbose;

                boards_count++;
                break;
            }
        }
        return;
    }

    for (dev = pacc->devices; dev != NULL; dev = dev->next) {
        board = &boards[boards_count];

        if (dev->vendor_id == VENDORID_MESAPCI) {
            if (dev->device_id == DEVICEID_MESA4I74) {
                board->type = BOARD_PCI;
                strncpy((char *) board->llio.board_name, "4I74", 4);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P3";
                board->llio.ioport_connector_name[2] = "P4";
                board->llio.fpga_part_number = "6slx9pq144";
                board->llio.num_leds = 0;
                board->llio.read = &pci_read;
                board->llio.write = &pci_write;
                board->llio.write_flash = &local_write_flash;
                board->llio.verify_flash = &local_verify_flash;
                board->llio.private = board;

                board->open = &pci_board_open;
                board->close = &pci_board_close;
                board->print_info = &pci_print_info;
                board->mem_base = dev->base_addr[0] & PCI_ADDR_MEM_MASK;
                board->len = dev->size[0];
                board->dev = dev;
                board->flash = BOARD_FLASH_HM2;
                board->fallback_support = 1;
                board->llio.verbose = access->verbose;

                boards_count++;
            } else if (dev->device_id == DEVICEID_MESA5I25) {
                board->type = BOARD_PCI;
                strncpy((char *) board->llio.board_name, "5I25", 4);
                board->llio.num_ioport_connectors = 2;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "P3";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.fpga_part_number = "6slx9pq144";
                board->llio.num_leds = 2;
                board->llio.read = &pci_read;
                board->llio.write = &pci_write;
                board->llio.write_flash = &local_write_flash;
                board->llio.verify_flash = &local_verify_flash;
                board->llio.private = board;

                board->open = &pci_board_open;
                board->close = &pci_board_close;
                board->print_info = &pci_print_info;
                board->mem_base = dev->base_addr[0] & PCI_ADDR_MEM_MASK;
                board->len = dev->size[0];
                board->dev = dev;
                board->flash = BOARD_FLASH_HM2;
                board->fallback_support = 1;
                board->llio.verbose = access->verbose;

                boards_count++;
            } else if (dev->device_id == DEVICEID_MESA6I25) {
                board->type = BOARD_PCI;
                strncpy(board->llio.board_name, "6I25", 4);
                board->llio.num_ioport_connectors = 2;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "P3";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.fpga_part_number = "6slx9pq144";
                board->llio.num_leds = 2;
                board->llio.read = &pci_read;
                board->llio.write = &pci_write;
                board->llio.write_flash = &local_write_flash;
                board->llio.verify_flash = &local_verify_flash;
                board->llio.private = board;

                board->open = &pci_board_open;
                board->close = &pci_board_close;
                board->print_info = &pci_print_info;
                board->mem_base = dev->base_addr[0] & PCI_ADDR_MEM_MASK;
                board->len = dev->size[0];
                board->dev = dev;
                board->flash = BOARD_FLASH_HM2;
                board->fallback_support = 1;
                board->llio.verbose = access->verbose;

                boards_count++;
            }
        } else if (dev->vendor_id == VENDORID_PLX) {
            if (dev->device_id == DEVICEID_PLX9030) {
                u16 ssid = pci_read_word(dev, PCI_SUBSYSTEM_ID);
                if (ssid == SUBDEVICEID_MESA5I20) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "5I20", 4);
                    board->llio.num_ioport_connectors = 3;
                    board->llio.pins_per_connector = 24;
                    board->llio.ioport_connector_name[0] = "P2";
                    board->llio.ioport_connector_name[1] = "P3";
                    board->llio.ioport_connector_name[2] = "P4";
                    board->llio.fpga_part_number = "2s200pq208";
                    board->llio.num_leds = 8;
                    board->llio.read = &pci_read;
                    board->llio.write = &pci_write;
                    board->llio.program_fpga = &plx9030_program_fpga;
                    board->llio.reset = &plx9030_reset;
                    board->llio.private = board;

                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->mem_base = dev->base_addr[5] & PCI_ADDR_MEM_MASK;
                    board->len = dev->size[5];
                    board->ctrl_base_addr = dev->base_addr[1] & PCI_ADDR_IO_MASK;
                    board->data_base_addr = dev->base_addr[2] & PCI_ADDR_IO_MASK;
                    board->dev = dev;
                    board->flash = BOARD_FLASH_NONE;
                    board->llio.verbose = access->verbose;

                    boards_count++;
                } else if (ssid == SUBDEVICEID_MESA4I65) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "4I65", 4);
                    board->llio.num_ioport_connectors = 3;
                    board->llio.pins_per_connector = 24;
                    board->llio.ioport_connector_name[0] = "P1";
                    board->llio.ioport_connector_name[1] = "P3";
                    board->llio.ioport_connector_name[2] = "P4";
                    board->llio.fpga_part_number = "2s200pq208";
                    board->llio.num_leds = 8;
                    board->llio.read = &pci_read;
                    board->llio.write = &pci_write;
                    board->llio.program_fpga = &plx9030_program_fpga;
                    board->llio.reset = &plx9030_reset;
                    board->llio.private = board;

                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->mem_base = dev->base_addr[5] & PCI_ADDR_MEM_MASK;
                    board->len = dev->size[5];
                    board->ctrl_base_addr = dev->base_addr[1] & PCI_ADDR_IO_MASK;
                    board->data_base_addr = dev->base_addr[2] & PCI_ADDR_IO_MASK;
                    board->dev = dev;
                    board->flash = BOARD_FLASH_NONE;
                    board->llio.verbose = access->verbose;

                    boards_count++;
                }
            } else if (dev->device_id == DEVICEID_PLX9054) {
                u16 ssid = pci_read_word(dev, PCI_SUBSYSTEM_ID);
                if ((ssid == SUBDEVICEID_MESA4I68_OLD) || (ssid == SUBDEVICEID_MESA4I68)) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "4I68", 4);
                    board->llio.num_ioport_connectors = 3;
                    board->llio.pins_per_connector = 24;
                    board->llio.ioport_connector_name[0] = "P1";
                    board->llio.ioport_connector_name[1] = "P2";
                    board->llio.ioport_connector_name[2] = "P4";
                    board->llio.fpga_part_number = "3s400pq208";
                    board->llio.num_leds = 4;
                    board->llio.read = &pci_read;
                    board->llio.write = &pci_write;
                    board->llio.program_fpga = &plx905x_program_fpga;
                    board->llio.reset = &plx905x_reset;
                    board->llio.private = board;

                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->mem_base = dev->base_addr[3] & PCI_ADDR_MEM_MASK;
                    board->len = dev->size[3];
                    board->ctrl_base_addr = dev->base_addr[1] & PCI_ADDR_IO_MASK;
                    board->data_base_addr = dev->base_addr[2] & PCI_ADDR_IO_MASK;
                    board->dev = dev;
                    board->flash = BOARD_FLASH_NONE;
                    board->llio.verbose = access->verbose;

                    boards_count++;
                } else if (ssid == SUBDEVICEID_MESA5I21) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "5I21", 4);
                    board->llio.num_ioport_connectors = 2;
                    board->llio.pins_per_connector = 32;
                    board->llio.ioport_connector_name[0] = "P1";
                    board->llio.ioport_connector_name[1] = "P1";
                    board->llio.fpga_part_number = "3s400pq208";
                    board->llio.num_leds = 8;
                    board->llio.read = &pci_read;
                    board->llio.write = &pci_write;
                    board->llio.program_fpga = &plx905x_program_fpga;
                    board->llio.reset = &plx905x_reset;
                    board->llio.private = board;

                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->mem_base = dev->base_addr[3] & PCI_ADDR_MEM_MASK;
                    board->len = dev->size[3];
                    board->ctrl_base_addr = dev->base_addr[1] & PCI_ADDR_IO_MASK;
                    board->data_base_addr = dev->base_addr[2] & PCI_ADDR_IO_MASK;
                    board->dev = dev;
                    board->flash = BOARD_FLASH_NONE;
                    board->llio.verbose = access->verbose;

                    boards_count++;
                } else if ((ssid == SUBDEVICEID_MESA5I22_10) || (ssid == SUBDEVICEID_MESA5I22_15)) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "5I22", 4);
                    board->llio.num_ioport_connectors = 4;
                    board->llio.pins_per_connector = 24;
                    board->llio.ioport_connector_name[0] = "P2";
                    board->llio.ioport_connector_name[1] = "P3";
                    board->llio.ioport_connector_name[2] = "P4";
                    board->llio.ioport_connector_name[3] = "P5";
                    if (ssid == SUBDEVICEID_MESA5I22_10) {
                        board->llio.fpga_part_number = "3s1000fg320";
                    } else if (ssid == SUBDEVICEID_MESA5I22_15) {
                        board->llio.fpga_part_number = "3s1500fg320";
                    }
                    board->llio.num_leds = 8;
                    board->llio.read = &pci_read;
                    board->llio.write = &pci_write;
                    board->llio.program_fpga = &plx905x_program_fpga;
                    board->llio.reset = &plx905x_reset;
                    board->llio.private = board;

                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->mem_base = dev->base_addr[3] & PCI_ADDR_MEM_MASK;
                    board->len = dev->size[3];
                    board->ctrl_base_addr = dev->base_addr[1] & PCI_ADDR_IO_MASK;
                    board->data_base_addr = dev->base_addr[2] & PCI_ADDR_IO_MASK;
                    board->dev = dev;
                    board->flash = BOARD_FLASH_NONE;
                    board->llio.verbose = access->verbose;

                    boards_count++;
                } else if (ssid == SUBDEVICEID_MESA5I23) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "5I23", 4);
                    board->llio.num_ioport_connectors = 3;
                    board->llio.pins_per_connector = 24;
                    board->llio.ioport_connector_name[0] = "P2";
                    board->llio.ioport_connector_name[1] = "P3";
                    board->llio.ioport_connector_name[2] = "P4";
                    board->llio.fpga_part_number = "3s400pq208";
                    board->llio.num_leds = 2;
                    board->llio.read = &pci_read;
                    board->llio.write = &pci_write;
                    board->llio.program_fpga = &plx905x_program_fpga;
                    board->llio.reset = &plx905x_reset;
                    board->llio.private = board;

                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->mem_base = dev->base_addr[3] & PCI_ADDR_MEM_MASK;
                    board->len = dev->size[3];
                    board->ctrl_base_addr = dev->base_addr[1] & PCI_ADDR_IO_MASK;
                    board->data_base_addr = dev->base_addr[2] & PCI_ADDR_IO_MASK;
                    board->dev = dev;
                    board->flash = BOARD_FLASH_NONE;
                    board->llio.verbose = access->verbose;

                    boards_count++;
                } else if ((ssid == SUBDEVICEID_MESA4I69_16) || (ssid == SUBDEVICEID_MESA4I69_25)) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "4I69", 4);
                    board->llio.num_ioport_connectors = 3;
                    board->llio.pins_per_connector = 24;
                    board->llio.ioport_connector_name[0] = "P1";
                    board->llio.ioport_connector_name[1] = "P3";
                    board->llio.ioport_connector_name[2] = "P4";
                    if (ssid == SUBDEVICEID_MESA4I69_16) {
                        board->llio.fpga_part_number = "6slx16ftg256";
                    } else if (ssid == SUBDEVICEID_MESA4I69_25) {
                        board->llio.fpga_part_number = "6slx25ftg256";
                    }
                    board->llio.num_leds = 8;
                    board->llio.read = &pci_read;
                    board->llio.write = &pci_write;
                    board->llio.program_fpga = &plx905x_program_fpga;
                    board->llio.reset = &plx905x_reset;
                    board->llio.private = board;

                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->mem_base = dev->base_addr[3] & PCI_ADDR_MEM_MASK;
                    board->len = dev->size[3];
                    board->ctrl_base_addr = dev->base_addr[1] & PCI_ADDR_IO_MASK;
                    board->data_base_addr = dev->base_addr[2] & PCI_ADDR_IO_MASK;
                    board->dev = dev;
                    board->flash = BOARD_FLASH_NONE;
                    board->llio.verbose = access->verbose;

                    boards_count++;
                }
            } else if (dev->device_id == DEVICEID_PLX9056) {
                u16 ssid = pci_read_word(dev, PCI_SUBSYSTEM_ID);
                if ((ssid == SUBDEVICEID_MESA3X20_10) || (ssid == SUBDEVICEID_MESA3X20_15) || (ssid == SUBDEVICEID_MESA3X20_20)) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "3X20", 4);
                    board->llio.num_ioport_connectors = 6;
                    board->llio.pins_per_connector = 24;
                    board->llio.ioport_connector_name[0] = "P4";
                    board->llio.ioport_connector_name[1] = "P5";
                    board->llio.ioport_connector_name[2] = "P6";
                    board->llio.ioport_connector_name[3] = "P9";
                    board->llio.ioport_connector_name[4] = "P6";
                    board->llio.ioport_connector_name[5] = "P7";
                    if (ssid == SUBDEVICEID_MESA3X20_10) {
                        board->llio.fpga_part_number = "3s1000fg456";
                    } else if (ssid == SUBDEVICEID_MESA3X20_15) {
                        board->llio.fpga_part_number = "3s1500fg456";
                    } else if (ssid == SUBDEVICEID_MESA3X20_20) {
                        board->llio.fpga_part_number = "3s2000fg456";
                    }
                    board->llio.num_leds = 0;
                    board->llio.read = &pci_read;
                    board->llio.write = &pci_write;
                    board->llio.program_fpga = &plx905x_program_fpga;
                    board->llio.reset = &plx905x_reset;
                    board->llio.write_flash = &local_write_flash;
                    board->llio.verify_flash = &local_verify_flash;
                    board->llio.private = board;

                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->mem_base = dev->base_addr[3] & PCI_ADDR_MEM_MASK;
                    board->len = dev->size[3];
                    board->ctrl_base_addr = dev->base_addr[1] & PCI_ADDR_IO_MASK;
                    board->data_base_addr = dev->base_addr[2] & PCI_ADDR_IO_MASK;
                    board->dev = dev;
                    board->flash = BOARD_FLASH_IO;
                    board->fallback_support = 0;
                    board->llio.verbose = access->verbose;

                    boards_count++;
                }
            } else if (dev->device_id == DEVICEID_PLX8112) {
                    board->type = BOARD_PCI;
                    strncpy(board->llio.board_name, "5I71", 4);
                    board->llio.private = board;
                    board->open = &pci_board_open;
                    board->close = &pci_board_close;
                    board->print_info = &pci_print_info;
                    board->dev = dev;
                    board->llio.verbose = access->verbose;

                    boards_count++;
            }
        }
    }
}

void pci_print_info(board_t *board) {
    int i;

    printf("\nPCI device %s at %04X:%02X:%02X.%1X (%04X:%04X)\n", board->llio.board_name, board->dev->domain,
        board->dev->bus, board->dev->dev, board->dev->func, board->dev->vendor_id, board->dev->device_id);
    if (board->llio.verbose == 0)
        return;

    for (i = 0; i < 6; i++) {
        u32 flags = pci_read_long(board->dev, PCI_BASE_ADDRESS_0 + 4*i);
        if (board->dev->base_addr[i] != 0) {
            if (flags & PCI_BASE_ADDRESS_SPACE_IO) {
                printf("  Region %d: I/O at %04X [size=%04X]\n", i, (unsigned int) (board->dev->base_addr[i] & PCI_BASE_ADDRESS_IO_MASK), (unsigned int) board->dev->size[i]);
            }  else {
                printf("  Region %d: Memory at %08X [size=%08X]\n", i, (unsigned int) board->dev->base_addr[i], (unsigned int) board->dev->size[i]);
            }
        }
    }
    printf("Communication:\n");
    if (board->ctrl_base_addr > 0)
        printf("  Ctrl I/O addr: %04X\n", board->ctrl_base_addr);
    if (board->data_base_addr > 0)
        printf("  Data I/O addr: %04X\n", board->data_base_addr);
    if (board->mem_base > 0)
        printf("  Memory: %08X\n", board->mem_base);

    printf("Board info:\n");
    if (board->flash_id > 0)
        printf("  Flash size: %s (id: 0x%02X)\n", eeprom_get_flash_type(board->flash_id), board->flash_id);
    printf("  Connectors count: %d\n", board->llio.num_ioport_connectors);
    printf("  Pins per connector: %d\n", board->llio.pins_per_connector);
    printf("  Connectors names:");
    for (i = 0; i < board->llio.num_ioport_connectors; i++)
        printf(" %s", board->llio.ioport_connector_name[i]);
    printf("\n");
    printf("  FPGA type: %s\n", board->llio.fpga_part_number);
    printf("  Number of leds: %d\n", board->llio.num_leds);
}
