
#ifndef __PCI_BOARDS_H
#define __PCI_BOARDS_H

#include <pci/pci.h>

#include "hostmot2.h"

#define MAX_PCI_BOARDS 8

typedef struct {
    struct pci_dev *dev;
    void *base;
    int len;
    u8 flash_id;
    llio_t llio;
} pci_board_t;

void pci_boards_init();
void pci_boards_scan();
void pci_print_info(pci_board_t *board);

#endif

