
#ifndef __PCI_BOARDS_H
#define __PCI_BOARDS_H

#include <pci/pci.h>

#include "hostmot2.h"

#define MAX_PCI_BOARDS 8

//
// the LAS?BRD registers are in the PLX 9030
//
// The HostMot2 firmware needs the #READY bit (0x2) set in order to work,
// but the EEPROMs on some older 5i20 cards dont set them right.
// The driver detects this problem and fixes it up.
//

#define LAS0BRD_OFFSET 0x28
#define LAS1BRD_OFFSET 0x2C
#define LAS2BRD_OFFSET 0x30
#define LAS3BRD_OFFSET 0x34

#define LASxBRD_READY 0x2

//
// PLX 9030 (5i20, 4i65)
//

/* I/O registers */
#define CTRL_STAT_OFFSET	0x0054	/* 9030 GPIO register (region 1) */

 /* bit number in 9030 GPIO register */
#define GPIO_3_MASK		(1<<11)	/* GPIO 3 */
#define DONE_MASK		(1<<11)	/* GPIO 3 */
#define _INIT_MASK		(1<<14)	/* GPIO 4 */
#define _LED_MASK		(1<<17)	/* GPIO 5 */
#define GPIO_6_MASK		(1<<20)	/* GPIO 6 */
#define _WRITE_MASK		(1<<23)	/* GPIO 7 */
#define _PROGRAM_MASK		(1<<26)	/* GPIO 8 */

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

