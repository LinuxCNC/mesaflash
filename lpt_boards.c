
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
#include "lpt_boards.h"

static u8 file_buffer[SECTOR_SIZE];
lpt_board_t lpt_boards[MAX_LPT_BOARDS];
int boards_count;

static int parport_get(lpt_board_t *board, unsigned short base, unsigned short base_hi, unsigned int modes) {
    if (base_hi == 0)
       base_hi = base + 0x400;

    board->base = base;
    printf("Using direct parport at ioaddr=0x%x:0x%x\n", base, base_hi);
    return 0;
}

static void parport_release(lpt_board_t *board) {
}

static inline u8 lpt_epp_read_status(lpt_board_t *board) {
    u8 data = inb(board->base + LPT_EPP_STATUS_OFFSET);
    printf("read status 0x%02X\n", data);
    return data;
}

static inline void lpt_epp_write_status(lpt_board_t *board, u8 status_byte) {
    outb(status_byte, board->base + LPT_EPP_STATUS_OFFSET);
    printf("wrote status 0x%02X\n", status_byte);
}

static inline void lpt_epp_write_control(lpt_board_t *board, u8 control_byte) {
    outb(control_byte, board->base + LPT_EPP_CONTROL_OFFSET);
    printf("wrote control 0x%02X\n", control_byte);
}

static inline int lpt_epp_check_for_timeout(lpt_board_t *board) {
    return lpt_epp_read_status(board) & 0x01;
}

static int lpt_epp_clear_timeout(lpt_board_t *board) {
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

static inline void lpt_epp_addr8(lpt_board_t *board, u8 addr) {
    outb(addr, board->base + LPT_EPP_ADDRESS_OFFSET);
    printf("selected address 0x%02X\n", addr);
}

static inline void lpt_epp_addr16(lpt_board_t *board, u16 addr) {
    outb((addr & 0x00FF), board->base + LPT_EPP_ADDRESS_OFFSET);
    outb((addr >> 8),     board->base + LPT_EPP_ADDRESS_OFFSET);
    printf("selected address 0x%04X\n", addr);
}

static inline u8 lpt_epp_read8(lpt_board_t *board) {
    u8 data = inb(board->base + LPT_EPP_DATA_OFFSET);
    printf("read data 0x%02X\n", data);
    return data;
}

static inline u32 lpt_epp_read32(lpt_board_t *board) {
    u32 data;

    if (board->epp_wide) {
        data = inl(board->base + LPT_EPP_DATA_OFFSET);
        printf("read data 0x%08X\n", data);
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

static inline void lpt_epp_write8(lpt_board_t *board, u8 data) {
    outb(data, board->base + LPT_EPP_DATA_OFFSET);
    //printf("wrote data 0x%02X\n", data);
}

static inline void lpt_epp_write32(lpt_board_t *board, u32 data) {
    if (board->epp_wide) {
	outl(data, board->base + LPT_EPP_DATA_OFFSET);
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
    lpt_board_t *board = self->private;

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
    lpt_board_t *board = self->private;

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
    lpt_board_t *board = self->private;
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

// return 0 if the board has been reset, -errno if not
int lpt_reset(llio_t *self) {
    lpt_board_t *board = self->private;
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

void lpt_boards_init() {
}

void lpt_boards_scan() {
#ifdef __linux__
        lpt_board_t *board = &lpt_boards[boards_count];
        int r;

        iopl(3);
        board->epp_wide = 1;

        //
        // claim the I/O regions for the parport
        // 

        r = parport_get(board, 0xD800, 0, PARPORT_MODE_EPP);
        if(r < 0)
            return;

        // set up the parport for EPP
	if(board->base_hi) {
	    outb(0x94, board->base_hi + LPT_ECP_CONTROL_HIGH_OFFSET); // select EPP mode in ECR
        }

        //
        // when we get here, the parallel port is in epp mode and ready to go
        //

        // select the device and tell it to make itself ready for io
        lpt_epp_write_control(board, 0x04);  // set control lines and input mode
        lpt_epp_clear_timeout(board);

        board->llio.read = lpt_read;
        board->llio.write = lpt_write;
        board->llio.program_fpga = lpt_program_fpga;
        board->llio.reset = lpt_reset;

        board->llio.num_ioport_connectors = 2;
        board->llio.pins_per_connector = 24;
        board->llio.ioport_connector_name[0] = "P4";
        board->llio.ioport_connector_name[1] = "P3";
        board->llio.num_leds = 8;
        board->llio.private = &board;

        //
        // now we want to detect if this 7i43 has the big FPGA or the small one
        // 3s200tq144 for the small board
        // 3s400tq144 for the big
        //

        // make sure the CPLD is in charge of the parallel port
        //lpt_reset(&(board->llio));

        //  select CPLD data register
        lpt_epp_addr8(board, 0);

        if (lpt_epp_read8(board) & 0x01) {
            board->llio.fpga_part_number = "3s400tq144";
        } else {
            board->llio.fpga_part_number = "3s200tq144";
        }
        printf("detected FPGA '%s'\n", board->llio.fpga_part_number);

        //board->llio.program_fpga(&(board->llio), "../../Pulpit/svst4_4s.bit");

        lpt_epp_addr16(board, HM2_COOKIE_REG);
        u32 cookie = lpt_epp_read32(board);
        printf("cookie %X\n", cookie);

        printf("board at (ioaddr=0x%04X, ioaddr_hi=0x%04X, epp_wide %s) found\n",
            board->base, board->base_hi, (board->epp_wide ? "ON" : "OFF"));
#endif
}
