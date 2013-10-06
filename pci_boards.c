
#include <pci/pci.h>
#include <sys/mman.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "anyio.h"
#include "eeprom.h"
#include "bitfile.h"
#include "pci_boards.h"

static int memfd = -1;
struct pci_access *pacc;
pci_board_t pci_boards[MAX_PCI_BOARDS];
int boards_count;
static u8 file_buffer[SECTOR_SIZE];

static int plx9030_program_fpga(llio_t *self, char *bitfile_name) {
    pci_board_t *board = self->private;
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
    pci_board_t *board = self->private;
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
    pci_board_t *board = self->private;
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
    pci_board_t *board = self->private;
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

    return 0;
}

static int plx905x_reset(llio_t *self) {
    pci_board_t *board = self->private;
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
        } else if (dev->device_id == DEVICEID_PLX9030) {
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
                board->llio.program_fpga = &plx9030_program_fpga;
                board->llio.reset = &plx9030_reset;
                board->llio.private = board;
                board->len = dev->size[5];
                board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev->base_addr[5]);
                board->dev = dev;
                board->ctrl_base_addr = dev->base_addr[1];
                board->data_base_addr = dev->base_addr[2];
                printf("\nPCI device %s at %02X:%02X.%X (%04X:%04X)\n", board->llio.board_name, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);
                //pci_print_info(board);
                iopl(3);
                plx9030_fixup_LASxBRD_READY(&(board->llio));

                //board->llio.reset(&(board->llio));
                //board->llio.program_fpga(&(board->llio), "../../Pulpit/SVST8_4.BIT");
                hm2_read_idrom(&(board->llio));

                boards_count++;
            }
        } else if (dev->device_id == DEVICEID_PLX9054) {
            u16 ssid = pci_read_word(dev, PCI_SUBSYSTEM_ID);
            if (ssid == SUBDEVICEID_MESA5I21) {
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
                board->len = dev->size[3];
                board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev->base_addr[3]);
                board->dev = dev;
                board->ctrl_base_addr = dev->base_addr[1];
                board->data_base_addr = dev->base_addr[2];
                printf("\nPCI device %s at %02X:%02X.%X (%04X:%04X)\n", board->llio.board_name, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);
                pci_print_info(board);
                iopl(3);

                board->llio.reset(&(board->llio));
                board->llio.program_fpga(&(board->llio), "../../Pulpit/I21LOOP.BIT");
                hm2_read_idrom(&(board->llio));

                boards_count++;
            }
        } else if (dev->device_id == DEVICEID_PLX9056) {
            u16 ssid = pci_read_word(dev, PCI_SUBSYSTEM_ID);
            if ((ssid == SUBDEVICEID_MESA3X20_10) || (ssid == SUBDEVICEID_MESA3X20_15) || (ssid == SUBDEVICEID_MESA3X20_20)) {
                strncpy(board->llio.board_name, "3x20", 4);
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
                board->llio.private = board;
                board->len = dev->size[3];
                board->base = mmap(0, board->len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev->base_addr[3]);
                board->dev = dev;
                board->ctrl_base_addr = dev->base_addr[1];
                board->data_base_addr = dev->base_addr[2];
                printf("\nPCI device %s at %02X:%02X.%X (%04X:%04X)\n", board->llio.board_name, dev->bus, dev->dev, dev->func, dev->vendor_id, dev->device_id);
                pci_print_info(board);
                iopl(3);

                board->llio.reset(&(board->llio));
                board->llio.program_fpga(&(board->llio), "../../Pulpit/SV24S.BIT");
                hm2_read_idrom(&(board->llio));

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
