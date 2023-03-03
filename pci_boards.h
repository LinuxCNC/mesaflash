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

#ifndef __PCI_BOARDS_H
#define __PCI_BOARDS_H

#include "types.h"
#include "boards.h"

#define VENDORID_MESAPCI         0x2718
#define VENDORID_PLX             0x10B5
#define VENDORID_XIO2001         0x104C

#define DEVICEID_MESA4I74        0x4174
#define DEVICEID_MESA5I24        0x5124
#define DEVICEID_MESA5I25        0x5125
#define DEVICEID_MESA5I25T       0x5925
#define DEVICEID_MESA6I24        0x6124
#define DEVICEID_MESA6I25        0x6125
#define DEVICEID_PLX9030         0x9030
#define DEVICEID_PLX9054         0x9054
#define DEVICEID_PLX9056         0x9056
#define DEVICEID_PLX8112         0x8112
#define DEVICEID_XIO2001         0x8240

#define SUBDEVICEID_MESA5I20     0x3131
#define SUBDEVICEID_MESA4I65     0x3132
#define SUBDEVICEID_MESA4I68_OLD 0x3133
#define SUBDEVICEID_MESA4I68     0x3311
#define SUBDEVICEID_MESA5I21     0x3312
#define SUBDEVICEID_MESA5I22_15  0x3313
#define SUBDEVICEID_MESA5I22_10  0x3314
#define SUBDEVICEID_MESA5I23     0x3315
#define SUBDEVICEID_MESA3X20_10  0x3427
#define SUBDEVICEID_MESA3X20_15  0x3428
#define SUBDEVICEID_MESA3X20_20  0x3429
#define SUBDEVICEID_MESA4I69_16  0x3472
#define SUBDEVICEID_MESA4I69_25  0x3473

#define EEPROM_93C66_SIZE     256
#define EEPROM_93C66_CMD_LEN  3
#define EEPROM_93C66_ADDR_LEN 8
#define EEPROM_93C66_DATA_LEN 16

#define EEPROM_93C66_CMD_WREN  4
#define EEPROM_93C66_CMD_WRITE 5
#define EEPROM_93C66_CMD_READ  6
#define EEPROM_93C66_CMD_ERASE 7

#define EEPROM_93C66_CMD_MASK 0x04
#define EEPROM_93C66_ADDR_MASK 0x80
#define EEPROM_93C66_DATA_MASK 0x8000

//
// PLX 9030 (5i20, 4i65)
//

// I/O registers
#define PLX9030_CTRL_INIT_OFFSET 0x0052
#define PLX9030_CTRL_STAT_OFFSET 0x0054    /* 9030 GPIO register (region 1) */

// bit number in 9030 INIT register
#define PLX9030_EECLK_MASK       0x0100
#define PLX9030_EECS_MASK        0x0200
#define PLX9030_EEDI_MASK        0x0400
#define PLX9030_EEDO_MASK        0x0800

// bit number in 9030 GPIO register
#define PLX9030_GPIO_3_MASK      (1<<11)    /* GPIO 3 */
#define PLX9030_DONE_MASK        (1<<11)    /* GPIO 3 */
#define PLX9030_INIT_MASK        (1<<14)    /* GPIO 4 */
#define PLX9030_LED_MASK         (1<<17)    /* GPIO 5 */
#define PLX9030_GPIO_6_MASK      (1<<20)    /* GPIO 6 */
#define PLX9030_WRITE_MASK       (1<<23)    /* GPIO 7 */
#define PLX9030_PROGRAM_MASK     (1<<26)    /* GPIO 8 */

// the LAS?BRD registers are in the PLX 9030
//
// The HostMot2 firmware needs the #READY bit (0x2) set in order to work,
// but the EEPROMs on some older 5i20 cards dont set them right.
// The driver detects this problem and fixes it up.
//

#define PLX9030_LAS0BRD_OFFSET 0x28
#define PLX9030_LAS1BRD_OFFSET 0x2C
#define PLX9030_LAS2BRD_OFFSET 0x30
#define PLX9030_LAS3BRD_OFFSET 0x34

#define PLX9030_LASxBRD_READY  0x2

//
// PLX 9054 (5i22, 5i23, 4i68, 4i69)
// Note: also used for the PLX 9056 (3x20)
//

// I/O register indices.
#define PLX905X_CTRL_STAT_OFFSET 0x006C     // 32-bit control/status register.
#define PLX9056_VPD_ADDR         0x4E
#define PLX9056_VPD_DATA         0x50
#define PLX9056_VPD_FMASK        0x8000     // F bit in VPD address register

// bit number in 9054 GPIO register
// yes, the direction control bits are not in the same order as the I/O bits
#define PLX905X_DONE_MASK        (1<<17)    // GPI
#define PLX905X_PROGRAM_MASK     (1<<16)    // GPO, active low
#define PLX905X_DONE_ENABLE      (1<<18)    // GPI direction control, 1=input
#define PLX905X_PROG_ENABLE      (1<<19)    // GPO direction control, 1=output

/* how long should we wait for DONE when programming 9054-based cards */
#define PLX905X_DONE_WAIT        20000

int pci_boards_init(board_access_t *access);
void pci_boards_cleanup(board_access_t *access);
void pci_boards_scan(board_access_t *access);
void pci_print_info(board_t *board);

#endif

