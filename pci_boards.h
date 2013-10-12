
#ifndef __PCI_BOARDS_H
#define __PCI_BOARDS_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "hostmot2.h"

#define MAX_PCI_BOARDS 8

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

// bit number in 9054 GPIO register
// yes, the direction control bits are not in the same order as the I/O bits
#define PLX905X_DONE_MASK        (1<<17)    // GPI
#define PLX905X_PROGRAM_MASK     (1<<16)    // GPO, active low
#define PLX905X_DONE_ENABLE      (1<<18)    // GPI direction control, 1=input
#define PLX905X_PROG_ENABLE      (1<<19)    // GPO direction control, 1=output

/* how long should we wait for DONE when programming 9054-based cards */
#define PLX905X_DONE_WAIT        20000

typedef struct {
    struct pci_dev *dev;
    void *base;
    int len;
    u16 ctrl_base_addr;
    u16 data_base_addr;
    u8 flash_id;
    llio_t llio;
} pci_board_t;

void pci_boards_init();
void pci_boards_scan();
void pci_print_info(pci_board_t *board);

#endif

