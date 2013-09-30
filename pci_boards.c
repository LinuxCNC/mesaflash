
#include <pci/pci.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "anyio.h"
#include "pci_boards.h"

static int memfd = -1;
struct pci_access *pacc;
pci_board_t pci_boards[MAX_PCI_BOARDS];
int boards_count;

int pci_read(llio_t *this, u32 addr, void *buffer, int size) {
    pci_board_t *board = this->private;

    memcpy(buffer, (board->base + addr), size);
    return 0;
}

int pci_write(llio_t *this, u32 addr, void *buffer, int size) {
    pci_board_t *board = this->private;

    memcpy((board->base + addr), buffer, size);
    return 0;
}

void pci_boards_init() {
    int eno;

    pacc = pci_alloc();
    pci_init(pacc);            // inicjowanie biblioteki libpci

    if (seteuid(0) != 0) {
        printf("%s need root privilges (or setuid root)", __func__);
        return;
    }
    memfd = open("/dev/mem", O_RDWR);
    eno = errno;
    seteuid(getuid());
    if (memfd < 0) {
        printf("%s can't open /dev/mem: %s", __func__, strerror(eno));
    }
}

void pci_boards_scan() {
    struct pci_dev *dev;

    pci_scan_bus(pacc);
    for (dev = pacc->devices; dev != NULL; dev = dev->next) {
        pci_board_t *board = &pci_boards[boards_count];

        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES);     // first run - fill data struct 
        if ((dev->vendor_id == VENDORID_MESAPCI) && (dev->device_id == DEVICEID_MESA5I25)) {
            strncpy((char *) board->llio.board_name, "5I25", 4);
            board->llio.num_ioport_connectors = 2;
            board->llio.pins_per_connector = 17;
            board->llio.ioport_connector_name[0] = "P3";
            board->llio.ioport_connector_name[1] = "P2";
            board->llio.fpga_part_number = "6slx9pq144";
            board->llio.num_leds = 2;
            board->llio.read = &pci_read;
            board->llio.write = &pci_write;
            board->len = dev->size[0];
            board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev->base_addr[0]);
            board->dev = dev;
            printf("PCI device: %s at %02X:%02X.%X (%04X:%04X)\n", board->llio.board_name, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);
        } else if ((dev->vendor_id == VENDORID_MESAPCI) && (dev->device_id == DEVICEID_MESA6I25)) {
            strncpy(board->llio.board_name, "6I25", 4);
            board->llio.num_ioport_connectors = 2;
            board->llio.pins_per_connector = 17;
            board->llio.ioport_connector_name[0] = "P3";
            board->llio.ioport_connector_name[1] = "P2";
            board->llio.fpga_part_number = "6slx9pq144";
            board->llio.num_leds = 2;
            board->llio.read = &pci_read;
            board->llio.write = &pci_write;
            board->len = dev->size[0];
            board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev->base_addr[0]);
            board->dev = dev;
            printf("PCI device: %s at %02X:%02X.%X (%04X:%04X)\n", board->llio.board_name, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);
        }
    }
}
