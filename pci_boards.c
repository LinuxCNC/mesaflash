
#include <pci/pci.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "anyio.h"
#include "eeprom.h"
#include "pci_boards.h"

static int memfd = -1;
struct pci_access *pacc;
pci_board_t pci_boards[MAX_PCI_BOARDS];
int boards_count;

int pci_read(llio_t *self, u32 addr, void *buffer, int size) {
    pci_board_t *board = self->private;

    memcpy(buffer, (board->base + addr), size);
    return 0;
}

int pci_write(llio_t *self, u32 addr, void *buffer, int size) {
    pci_board_t *board = self->private;

    memcpy((board->base + addr), buffer, size);
    return 0;
}

int pci_program_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    return eeprom_write_area(self, bitfile_name, start_address);
}

int pci_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    return eeprom_verify_area(self, bitfile_name, start_address);
}

int pci_program_fpga(llio_t *self, char *bitfile_name) {
    return 0;
}

int pci_reset(llio_t *self) {
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

        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_IRQ | PCI_FILL_BASES | PCI_FILL_ROM_BASE | PCI_FILL_SIZES | PCI_FILL_CLASS);     // first run - fill data struct 
        if (dev->vendor_id == VENDORID_MESAPCI) {
            if (dev->device_id == DEVICEID_MESA5I25) {
                strncpy((char *) board->llio.board_name, "5I25", 4);
                board->llio.num_ioport_connectors = 2;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "P3";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.fpga_part_number = "6slx9pq144";
                board->llio.num_leds = 2;
                board->llio.read = &pci_read;
                board->llio.write = &pci_write;
                board->llio.program_flash = &pci_program_flash;
                board->llio.private = board;
                board->len = dev->size[0];
                board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev->base_addr[0]);
                board->dev = dev;
                board->flash_id = read_flash_id(&(board->llio));
                printf("\nPCI device %s at %02X:%02X.%X (%04X:%04X)\n", board->llio.board_name, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);
                pci_print_info(board);
                //hm2_read_idrom(&(board->llio));

                pci_verify_flash(&(board->llio), "../../Pulpit/7i77x2.bit", 0x80000);

                boards_count++;
            } else if (dev->device_id == DEVICEID_MESA6I25) {
                strncpy(board->llio.board_name, "6I25", 4);
                board->llio.num_ioport_connectors = 2;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "P3";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.fpga_part_number = "6slx9pq144";
                board->llio.num_leds = 2;
                board->llio.read = &pci_read;
                board->llio.write = &pci_write;
                board->llio.program_flash = &pci_program_flash;
                board->llio.private = board;
                board->len = dev->size[0];
                board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev->base_addr[0]);
                board->dev = dev;
                board->flash_id = read_flash_id(&(board->llio));
                printf("\nPCI device %s at %02X:%02X.%X (%04X:%04X)\n", board->llio.board_name, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);

                boards_count++;
            }
        } else if ((dev->vendor_id == VENDORID_PLX) && (dev->device_id == DEVICEID_PLX9030)) {
            u16 ssid = pci_read_word(dev, PCI_SUBSYSTEM_ID);
            if (ssid == SUBDEVICEID_MESA5I20) {
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
                board->llio.program_fpga = &pci_program_fpga;
                board->llio.reset = &pci_reset;
                board->llio.private = board;
                board->len = dev->size[5];
                board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev->base_addr[5]);
                board->dev = dev;
                board->ctrl_base_addr = dev->base_addr[1];
                board->data_base_addr = dev->base_addr[2];
                printf("\nPCI device %s at %02X:%02X.%X (%04X:%04X)\n", board->llio.board_name, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);
                pci_print_info(board);

                boards_count++;
            }
        }
    }
}

void pci_print_info(pci_board_t *board) {
    int i;

    for (i = 0; i < 6; i++) {
        u32 flags = pci_read_long(board->dev, PCI_BASE_ADDRESS_0 + 4*i);
        if (board->dev->base_addr[i] != 0) {
            if (flags & PCI_BASE_ADDRESS_SPACE_IO) {
                printf("  Region %d: I/O at %04X [size=%u]\n", i, (unsigned int) board->dev->base_addr[i], (unsigned int) board->dev->size[i]);
            }  else {
                printf("  Region %d: Memory at %08X [size=%u]\n", i, (unsigned int) board->dev->base_addr[i], (unsigned int) board->dev->size[i]);
            }
        }
    }
    if (board->flash_id > 0) {
        printf("  flash id: 0x%02X %s\n", board->flash_id, eeprom_get_flash_type(board->flash_id));
    }
}
