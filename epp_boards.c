
#ifdef __linux__
#include <pci/pci.h>
#include <linux/parport.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "common.h"
#include "spi_eeprom.h"
#include "bitfile.h"
#include "anyio.h"
#include "epp_boards.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;
static u8 page_buffer[PAGE_SIZE];
extern u8 boot_block[BOOT_BLOCK_SIZE];
static u8 file_buffer[SECTOR_SIZE];

static int parport_get(board_t *board, unsigned short base_lo, unsigned short base_hi, unsigned int modes) {
    if (base_hi == 0)
       base_hi = base_lo + 0x400;

    board->base_lo = base_lo;
    board->base_hi = base_hi;
    //printf("Using direct parport at ioaddr=0x%x:0x%x\n", base_lo, base_hi);
    return 0;
}

static void parport_release(board_t *board) {
}

static inline u8 epp_read_status(board_t *board) {
    u8 data = inb(board->base_lo + EPP_STATUS_OFFSET);
//    printf("read status 0x%02X\n", data);
    return data;
}

static inline void epp_write_status(board_t *board, u8 status_byte) {
    outb(status_byte, board->base_lo + EPP_STATUS_OFFSET);
//    printf("wrote status 0x%02X\n", status_byte);
}

static inline void epp_write_control(board_t *board, u8 control_byte) {
    outb(control_byte, board->base_lo + EPP_CONTROL_OFFSET);
//    printf("wrote control 0x%02X\n", control_byte);
}

static inline int epp_check_for_timeout(board_t *board) {
    return epp_read_status(board) & 0x01;
}

static int epp_clear_timeout(board_t *board) {
    u8 status;

    if (!epp_check_for_timeout(board)) {
        return 1;
    }

    /* To clear timeout some chips require double read */
    epp_read_status(board);

    // read in the actual status register
    status = epp_read_status(board);

    epp_write_status(board, status | 0x01);  // Some reset by writing 1
    epp_write_status(board, status & 0xFE);  // Others by writing 0

    if (epp_check_for_timeout(board)) {
        printf("failed to clear EPP Timeout!\n");
        return 0;  // fail
    }
    return 1;  // success
}

static inline void epp_addr8(board_t *board, u8 addr) {
    outb(addr, board->base_lo + EPP_ADDRESS_OFFSET);
//    printf("selected address 0x%02X\n", addr);
}

static inline void epp_addr16(board_t *board, u16 addr) {
    outb((addr & 0x00FF), board->base_lo + EPP_ADDRESS_OFFSET);
    outb((addr >> 8),     board->base_lo + EPP_ADDRESS_OFFSET);
//    printf("selected address 0x%04X\n", addr);
}

static inline u8 epp_read8(board_t *board) {
    u8 data = inb(board->base_lo + EPP_DATA_OFFSET);
//    printf("read data 0x%02X\n", data);
    return data;
}

static inline u32 epp_read32(board_t *board) {
    u32 data;

    if (board->epp_wide) {
        data = inl(board->base_lo + EPP_DATA_OFFSET);
//        printf("read data 0x%08X\n", data);
    } else {
        u8 a, b, c, d;
        a = epp_read8(board);
        b = epp_read8(board);
        c = epp_read8(board);
        d = epp_read8(board);
        data = a + (b << 8) + (c << 16) + (d << 24);
    }

    return data;
}

static inline void epp_write8(board_t *board, u8 data) {
    outb(data, board->base_lo + EPP_DATA_OFFSET);
    //printf("wrote data 0x%02X\n", data);
}

static inline void epp_write32(board_t *board, u32 data) {
    if (board->epp_wide) {
    outl(data, board->base_lo + EPP_DATA_OFFSET);
    //    printf("wrote data 0x%08X\n", data);
    } else {
        epp_write8(board, (data) & 0xFF);
        epp_write8(board, (data >>  8) & 0xFF);
        epp_write8(board, (data >> 16) & 0xFF);
        epp_write8(board, (data >> 24) & 0xFF);
    }
}

int epp_read(llio_t *self, u32 addr, void *buffer, int size) {
    int bytes_remaining = size;
    board_t *board = self->private;

    epp_addr16(board, addr | EPP_ADDR_AUTOINCREMENT);

    for (; bytes_remaining > 3; bytes_remaining -= 4) {
        *((u32*)buffer) = epp_read32(board);
        buffer += 4;
    }

    for ( ; bytes_remaining > 0; bytes_remaining --) {
        *((u8*)buffer) = epp_read8(board);
        buffer ++;
    }

    if (epp_check_for_timeout(board)) {
        printf("EPP timeout on data cycle of read(addr=0x%04x, size=%d)\n", addr, size);
    //    self->needs_reset = 1;
        epp_clear_timeout(board);
        return 0;  // fail
    }

    return 1;  // success
}

int epp_write(llio_t *self, u32 addr, void *buffer, int size) {
    int bytes_remaining = size;
    board_t *board = self->private;

    epp_addr16(board, addr | EPP_ADDR_AUTOINCREMENT);

    for (; bytes_remaining > 3; bytes_remaining -= 4) {
        epp_write32(board, *((u32*)buffer));
        buffer += 4;
    }

    for ( ; bytes_remaining > 0; bytes_remaining --) {
        epp_write8(board, *((u8*)buffer));
        buffer ++;
    }

    if (epp_check_for_timeout(board)) {
        printf("EPP timeout on data cycle of write(addr=0x%04x, size=%d)\n", addr, size);
     //   self->needs_reset = 1;
        epp_clear_timeout(board);
        return 0;
    }

    return 1;
}

int epp_program_fpga(llio_t *self, char *bitfile_name) {
    board_t *board = self->private;
    int bindex, bytesread;
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
    //
    // send the firmware
    //

    // select the CPLD's data address
    epp_addr8(board, 0);

    printf("Programming FPGA...\n");
    printf("  |");
    fflush(stdout);
    // program the FPGA
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            epp_write8(board, bitfile_reverse_bits(file_buffer[bindex]));
            bindex++;
        }
        printf("W");
        fflush(stdout);
    }

    printf("\n");
    fclose(fp);

    // see if it worked
    if (epp_check_for_timeout(board)) {
        printf("EPP Timeout while sending firmware!\n");
        return -EIO;
    }

    return 0;
}

int epp_program_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    return eeprom_write_area(self, bitfile_name, start_address);
}

int epp_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    return eeprom_verify_area(self, bitfile_name, start_address);
}

// return 0 if the board has been reset, -errno if not
int epp_reset(llio_t *self) {
    board_t *board = self->private;
    u8 byte;


    //
    // this resets the FPGA *only* if it's currently configured with the
    // HostMot2 or GPIO firmware
    //

    epp_addr16(board, 0x7F7F);
    epp_write8(board, 0x5A);
    epp_addr16(board, 0x7F7F);
    epp_write8(board, 0x5A);


    // 
    // this code resets the FPGA *only* if the CPLD is in charge of the
    // parallel port
    //

    // select the control register
    epp_addr8(board, 1);

    // bring the Spartan3's PROG_B line low for 1 us (the specs require 300-500 ns or longer)
    epp_write8(board, 0x00);
    sleep_ns(1000);

    // bring the Spartan3's PROG_B line high and wait for 2 ms before sending firmware (required by spec)
    epp_write8(board, 0x01);
    sleep_ns(2 * 1000 * 1000);

    // make sure the FPGA is not asserting its /DONE bit
    byte = epp_read8(board);
    if ((byte & 0x01) != 0) {
        printf("/DONE is not low after CPLD reset!\n");
        return -EIO;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////

int epp_boards_init(board_access_t *access) {
    return 0;
}

void epp_boards_scan(board_access_t *access) {
#ifdef __linux__
    board_t *board = &boards[boards_count];
    int r;
    u16 epp_addr;
    u32 cookie;

    iopl(3);
    board->epp_wide = 1;

    if (strncmp(access->dev_addr, "0x", 2) == 0) {
        access->dev_addr[0] = '0';
        access->dev_addr[1] = '0';
        epp_addr = strtol(access->dev_addr, NULL, 16);
    } else {
        epp_addr = strtol(access->dev_addr, NULL, 10);
    }

    r = parport_get(board, epp_addr, access->epp_base_hi_addr, PARPORT_MODE_EPP);
    if (r < 0)
       return;

    // set up the parport for EPP
    if(board->base_hi) {
        outb(0x94, board->base_hi + ECP_CONTROL_HIGH_OFFSET); // select EPP mode in ECR
    }

    // select the device and tell it to make itself ready for io
    epp_write_control(board, 0x04);  // set control lines and input mode
    epp_clear_timeout(board);

    epp_addr16(board, HM2_COOKIE_REG | EPP_ADDR_AUTOINCREMENT);
    cookie = epp_read32(board);
    epp_check_for_timeout(board);
    if (cookie == HM2_COOKIE) {
        u32 idrom_addr;
        u8 board_name[8];
        u32 *ptr = (u32 *) &board_name;

        epp_addr16(board, HM2_IDROM_ADDR | EPP_ADDR_AUTOINCREMENT);
        idrom_addr = epp_read32(board);
        epp_addr16(board, (idrom_addr + offsetof(hm2_idrom_desc_t, board_name)) | EPP_ADDR_AUTOINCREMENT);
        *ptr++ = epp_read32(board);
        *ptr = epp_read32(board);

        if (strncmp(board_name, "MESA7I90", 8) == 0) {
            board->type = BOARD_EPP;
            board->mode = BOARD_MODE_FPGA;
            strncpy(board->dev_addr, access->dev_addr, 16);
            strncpy(board->llio.board_name, "7I90HD", 16);

            board->llio.read = epp_read;
            board->llio.write = epp_write;
            board->llio.program_fpga = epp_program_fpga;
            board->llio.program_flash = epp_program_flash;
            board->llio.verify_flash = epp_verify_flash;

            board->llio.num_ioport_connectors = 3;
            board->llio.pins_per_connector = 24;
            board->llio.ioport_connector_name[0] = "P1";
            board->llio.ioport_connector_name[1] = "P2";
            board->llio.ioport_connector_name[2] = "P3";
            board->llio.num_leds = 2;
            board->llio.private = board;
            eeprom_init(&(board->llio), BOARD_FLASH_HM2);
            board->flash_id = read_flash_id(&(board->llio));
            prepare_boot_block(board->flash_id);
            board->flash_start_address = eeprom_calc_user_space(board->flash_id);
            board->llio.verbose = access->verbose;

            boards_count++;
        } else if (strncmp(board_name, "MESA7I43", 8) == 0) {
            board->type = BOARD_EPP;
            board->mode = BOARD_MODE_FPGA;
            strncpy(board->dev_addr, access->dev_addr, 16);
            strncpy(board->llio.board_name, "7I43", 16);

            board->llio.read = epp_read;
            board->llio.write = epp_write;
            board->llio.program_fpga = epp_program_fpga;
            board->llio.reset = epp_reset;

            board->llio.num_ioport_connectors = 2;
            board->llio.pins_per_connector = 24;
            board->llio.ioport_connector_name[0] = "P4";
            board->llio.ioport_connector_name[1] = "P3";
            board->llio.num_leds = 2;
            board->llio.private = board;
            board->llio.verbose = access->verbose;

            boards_count++;
        }
    } else {
        board->type = BOARD_EPP;
        board->mode = BOARD_MODE_CPLD;
        strncpy(board->dev_addr, access->dev_addr, 16);
        strncpy(board->llio.board_name, "7I43", 16);

        board->llio.read = epp_read;
        board->llio.write = epp_write;
        board->llio.program_fpga = epp_program_fpga;
        board->llio.reset = epp_reset;

        board->llio.num_ioport_connectors = 2;
        board->llio.pins_per_connector = 24;
        board->llio.ioport_connector_name[0] = "P4";
        board->llio.ioport_connector_name[1] = "P3";
        board->llio.num_leds = 2;
        board->llio.private = board;
        board->llio.verbose = access->verbose;

        boards_count++;
    }
#endif
}

void epp_print_info(board_t *board) {
    printf("\nEPP device %s at 0x%04X\n", board->llio.board_name, board->base_lo);
    if (board->llio.verbose == 0)
        return;
    if (board->mode == BOARD_MODE_CPLD)
        printf("  controlled by CPLD\n");
    else if (board->mode == BOARD_MODE_FPGA)
        printf("  controlled by FPGA\n");
    if (board->flash_id > 0)
        printf("  Flash size: %s (id: 0x%02X)\n", eeprom_get_flash_type(board->flash_id), board->flash_id);
}