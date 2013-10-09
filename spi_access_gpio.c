
#include <pci/pci.h>

#include "spi_access_gpio.h"
#include "pci_boards.h"
#include "hostmot2.h"

static u16 GPIO_reg_val;

static void set_cs_high(llio_t *self) {
    pci_board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val | 0x2;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_cs_low(llio_t *self) {
    pci_board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val & (~ 0x2);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_din_high(llio_t *self) {
    pci_board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val | 0x8;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_din_low(llio_t *self) {
    pci_board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val & (~0x8);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_clock_high(llio_t *self) {
    pci_board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val | 0x10;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void set_clock_low(llio_t *self) {
    pci_board_t *board = self->private;

    GPIO_reg_val = GPIO_reg_val & (~ 0x10);
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static int get_bit(llio_t *self) {
    pci_board_t *board = self->private;
    u16 data;

    data = pci_read_word(board->dev, XIO2001_GPIO_DATA_REG);
    if (data & 0x4)
        return 1;
    else
        return 0;
}

static void prefix(llio_t *self) {
    set_cs_high(self);
    set_din_low(self);
    set_clock_low(self);
    set_cs_low(self);
}

static void suffix(llio_t *self) {
    set_cs_high(self);
    set_din_low(self);
}

static void init_gpio(llio_t *self) {
    pci_board_t *board = self->private;

    pci_write_word(board->dev, XIO2001_GPIO_ADDR_REG, 0x001B);
    pci_write_word(board->dev, XIO2001_SBAD_STAT_REG, 0x0000);
    GPIO_reg_val = 0x0003;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
}

static void restore_gpio(llio_t *self) {
    pci_board_t *board = self->private;

    GPIO_reg_val = 0x0003;
    pci_write_word(board->dev, XIO2001_GPIO_DATA_REG, GPIO_reg_val);
    pci_write_word(board->dev, XIO2001_GPIO_ADDR_REG, 0x0000);
}

static void send_byte(llio_t *self, u8 byte) {
    u32 mask = DATA_MASK;
    int i;

    for (i = 0; i < CMD_LEN; i++) {
        if ((mask & byte) == 0)
            set_din_low(self);
        else
            set_din_high(self);
        mask >>= 1;
        set_clock_high(self);
        set_clock_low(self);
    }
}

static u8 recv_byte(llio_t *self) {
    u32 mask, data = 0;
    int i;

    mask = DATA_MASK;
    for (i = 0; i < DATA_LEN; i++) {
        if (get_bit(self) == 1)
            data |= mask;
        mask >>= 1;
        set_clock_high(self);
        set_clock_low(self);
    }
  return data;
}

void open_spi_access_gpio(llio_t *self, spi_eeprom_dev_t *access) {
    access->set_cs_low = &set_cs_low, 
    access->set_cs_high = &set_cs_high;
    access->prefix = &prefix;
    access->suffix = &suffix;
    access->send_byte = &send_byte;
    access->recv_byte = &recv_byte;

    init_gpio(self);
};

void close_spi_access_gpio(llio_t *self, spi_eeprom_dev_t *access) {
    restore_gpio(self);
}
