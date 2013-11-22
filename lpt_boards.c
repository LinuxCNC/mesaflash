
#ifdef __linux__
#include <pci/pci.h>
#include <linux/parport.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <stdio.h>
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
#include "lpt_boards.h"

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

static inline u8 lpt_epp_read_status(board_t *board) {
    u8 data = inb(board->base_lo + LPT_EPP_STATUS_OFFSET);
//    printf("read status 0x%02X\n", data);
    return data;
}

static inline void lpt_epp_write_status(board_t *board, u8 status_byte) {
    outb(status_byte, board->base_lo + LPT_EPP_STATUS_OFFSET);
//    printf("wrote status 0x%02X\n", status_byte);
}

static inline void lpt_epp_write_control(board_t *board, u8 control_byte) {
    outb(control_byte, board->base_lo + LPT_EPP_CONTROL_OFFSET);
//    printf("wrote control 0x%02X\n", control_byte);
}

static inline int lpt_epp_check_for_timeout(board_t *board) {
    return lpt_epp_read_status(board) & 0x01;
}

static int lpt_epp_clear_timeout(board_t *board) {
    u8 status;

    if (!lpt_epp_check_for_timeout(board)) {
        return 1;
    }

    /* To clear timeout some chips require double read */
    lpt_epp_read_status(board);

    // read in the actual status register
    status = lpt_epp_read_status(board);

    lpt_epp_write_status(board, status | 0x01);  // Some reset by writing 1
    lpt_epp_write_status(board, status & 0xFE);  // Others by writing 0

    if (lpt_epp_check_for_timeout(board)) {
        printf("failed to clear EPP Timeout!\n");
        return 0;  // fail
    }
    return 1;  // success
}

static inline void lpt_epp_addr8(board_t *board, u8 addr) {
    outb(addr, board->base_lo + LPT_EPP_ADDRESS_OFFSET);
//    printf("selected address 0x%02X\n", addr);
}

static inline void lpt_epp_addr16(board_t *board, u16 addr) {
    outb((addr & 0x00FF), board->base_lo + LPT_EPP_ADDRESS_OFFSET);
    outb((addr >> 8),     board->base_lo + LPT_EPP_ADDRESS_OFFSET);
//    printf("selected address 0x%04X\n", addr);
}

static inline u8 lpt_epp_read8(board_t *board) {
    u8 data = inb(board->base_lo + LPT_EPP_DATA_OFFSET);
//    printf("read data 0x%02X\n", data);
    return data;
}

static inline u32 lpt_epp_read32(board_t *board) {
    u32 data;

    if (board->epp_wide) {
        data = inl(board->base_lo + LPT_EPP_DATA_OFFSET);
//        printf("read data 0x%08X\n", data);
    } else {
        u8 a, b, c, d;
        a = lpt_epp_read8(board);
        b = lpt_epp_read8(board);
        c = lpt_epp_read8(board);
        d = lpt_epp_read8(board);
        data = a + (b << 8) + (c << 16) + (d << 24);
    }

    return data;
}

static inline void lpt_epp_write8(board_t *board, u8 data) {
    outb(data, board->base_lo + LPT_EPP_DATA_OFFSET);
    //printf("wrote data 0x%02X\n", data);
}

static inline void lpt_epp_write32(board_t *board, u32 data) {
    if (board->epp_wide) {
    outl(data, board->base_lo + LPT_EPP_DATA_OFFSET);
    //    printf("wrote data 0x%08X\n", data);
    } else {
        lpt_epp_write8(board, (data) & 0xFF);
        lpt_epp_write8(board, (data >>  8) & 0xFF);
        lpt_epp_write8(board, (data >> 16) & 0xFF);
        lpt_epp_write8(board, (data >> 24) & 0xFF);
    }
}

int lpt_read(llio_t *self, u32 addr, void *buffer, int size) {
    int bytes_remaining = size;
    board_t *board = self->private;

    lpt_epp_addr16(board, addr | LPT_ADDR_AUTOINCREMENT);

    for (; bytes_remaining > 3; bytes_remaining -= 4) {
        *((u32*)buffer) = lpt_epp_read32(board);
        buffer += 4;
    }

    for ( ; bytes_remaining > 0; bytes_remaining --) {
        *((u8*)buffer) = lpt_epp_read8(board);
        buffer ++;
    }

    if (lpt_epp_check_for_timeout(board)) {
        printf("EPP timeout on data cycle of read(addr=0x%04x, size=%d)\n", addr, size);
    //    self->needs_reset = 1;
        lpt_epp_clear_timeout(board);
        return 0;  // fail
    }

    return 1;  // success
}

int lpt_write(llio_t *self, u32 addr, void *buffer, int size) {
    int bytes_remaining = size;
    board_t *board = self->private;

    lpt_epp_addr16(board, addr | LPT_ADDR_AUTOINCREMENT);

    for (; bytes_remaining > 3; bytes_remaining -= 4) {
        lpt_epp_write32(board, *((u32*)buffer));
        buffer += 4;
    }

    for ( ; bytes_remaining > 0; bytes_remaining --) {
        lpt_epp_write8(board, *((u8*)buffer));
        buffer ++;
    }

    if (lpt_epp_check_for_timeout(board)) {
        printf("EPP timeout on data cycle of write(addr=0x%04x, size=%d)\n", addr, size);
     //   self->needs_reset = 1;
        lpt_epp_clear_timeout(board);
        return 0;
    }

    return 1;
}

int lpt_program_fpga(llio_t *self, char *bitfile_name) {
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
    lpt_epp_addr8(board, 0);

    printf("Programming FPGA...\n");
    printf("  |");
    fflush(stdout);
    // program the FPGA
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            lpt_epp_write8(board, bitfile_reverse_bits(file_buffer[bindex]));
            bindex++;
        }
        printf("W");
        fflush(stdout);
    }

    printf("\n");
    fclose(fp);

    // see if it worked
    if (lpt_epp_check_for_timeout(board)) {
        printf("EPP Timeout while sending firmware!\n");
        return -EIO;
    }

    return 0;
}

int lpt_program_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    return eeprom_write_area(self, bitfile_name, start_address);
}

int lpt_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    return eeprom_verify_area(self, bitfile_name, start_address);
}

// return 0 if the board has been reset, -errno if not
int lpt_reset(llio_t *self) {
    board_t *board = self->private;
    u8 byte;


    //
    // this resets the FPGA *only* if it's currently configured with the
    // HostMot2 or GPIO firmware
    //

    lpt_epp_addr16(board, 0x7F7F);
    lpt_epp_write8(board, 0x5A);
    lpt_epp_addr16(board, 0x7F7F);
    lpt_epp_write8(board, 0x5A);


    // 
    // this code resets the FPGA *only* if the CPLD is in charge of the
    // parallel port
    //

    // select the control register
    lpt_epp_addr8(board, 1);

    // bring the Spartan3's PROG_B line low for 1 us (the specs require 300-500 ns or longer)
    lpt_epp_write8(board, 0x00);
    sleep_ns(1000);

    // bring the Spartan3's PROG_B line high and wait for 2 ms before sending firmware (required by spec)
    lpt_epp_write8(board, 0x01);
    sleep_ns(2 * 1000 * 1000);

    // make sure the FPGA is not asserting its /DONE bit
    byte = lpt_epp_read8(board);
    if ((byte & 0x01) != 0) {
        printf("/DONE is not low after CPLD reset!\n");
        return -EIO;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////

int lpt_boards_init(board_access_t *access) {
    return 0;
}

void lpt_boards_scan(board_access_t *access) {
#ifdef __linux__
    board_t *board = &boards[boards_count];
    int r;
    u16 lpt_addr;
    u32 cookie;

    iopl(3);
    board->epp_wide = 1;

    if (strncmp(access->dev_addr, "0x", 2) == 0) {
        access->dev_addr[0] = '0';
        access->dev_addr[1] = '0';
        lpt_addr = strtol(access->dev_addr, NULL, 16);
    } else {
        lpt_addr = strtol(access->dev_addr, NULL, 10);
    }

    r = parport_get(board, lpt_addr, access->lpt_base_hi_addr, PARPORT_MODE_EPP);
    if (r < 0)
       return;

    // set up the parport for EPP
    if(board->base_hi) {
        outb(0x94, board->base_hi + LPT_ECP_CONTROL_HIGH_OFFSET); // select EPP mode in ECR
    }

    // select the device and tell it to make itself ready for io
    lpt_epp_write_control(board, 0x04);  // set control lines and input mode
    lpt_epp_clear_timeout(board);

    lpt_epp_addr16(board, HM2_COOKIE_REG | LPT_ADDR_AUTOINCREMENT);
    cookie = lpt_epp_read32(board);
    lpt_epp_check_for_timeout(board);
    if (cookie == HM2_COOKIE) {
        board->type = BOARD_LPT;
        strncpy(board->dev_addr, access->dev_addr, 16);
        strncpy(board->llio.board_name, "7I90HD", 16);

        board->llio.read = lpt_read;
        board->llio.write = lpt_write;
        board->llio.program_fpga = lpt_program_fpga;
        board->llio.program_flash = lpt_program_flash;
        board->llio.verify_flash = lpt_verify_flash;
        board->llio.reset = lpt_reset;

        board->llio.num_ioport_connectors = 3;
        board->llio.pins_per_connector = 24;
        board->llio.ioport_connector_name[0] = "P1";
        board->llio.ioport_connector_name[1] = "P2";
        board->llio.ioport_connector_name[2] = "P3";
        board->llio.num_leds = 2;
        board->llio.private = board;
        eeprom_init(&(board->llio));
        board->flash_start_address = 0x80000;
        board->llio.verbose = access->verbose;

        boards_count++;
    }
#endif
}

void lpt_print_info(board_t *board) {
    printf("\nLPT device %s at 0x%04X\n", board->llio.board_name, board->base_lo);
}
