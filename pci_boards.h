
#ifndef __PCI_BOARDS_H
#define __PCI_BOARDS_H

#include <pci/pci.h>

#include "hostmot2.h"

#define MAX_PCI_BOARDS 8

typedef struct {
    struct pci_dev *dev;
    void *base;
    int len;
    u32 ctrl_base_addr;
    u32 data_base_addr;
    llio_t llio;
} pci_board_t;

void pci_boards_init();
void pci_boards_scan();

#endif

