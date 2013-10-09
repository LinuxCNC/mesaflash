
#include <pci/pci.h>
#include <stdio.h>

#include "spi_access_hm2.h"
#include "hostmot2.h"

static void wait_for_data(llio_t *self) {
    u32 i = 0;
    u32 data = 0;

    for (i = 0; (((data & 0xFF) & DAV_MASK) == 0) && (i < 5000) ; i++) {
        self->read(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
    }
    if (i == 5000) {
        printf("%x timeout waiting for SPI data\n", data);
    }
}

static void hm2_set_cs_high(llio_t *self) {
    u32 data = 1;

    self->write(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
}

static void hm2_set_cs_low(llio_t *self) {
    u32 data = 0;

    self->write(self, HM2_SPI_CTRL_REG, &data, sizeof(data));
}

static void hm2_prefix(llio_t *self) {
    hm2_set_cs_low(self);
}

static void hm2_suffix(llio_t *self) {
    hm2_set_cs_high(self);
}

static void hm2_send_byte(llio_t *self, u8 byte) {
    u32 data = byte;

    self->write(self, HM2_SPI_DATA_REG, &data, sizeof(data));
    wait_for_data(self);
}

static u8 hm2_recv_byte(llio_t *self) {
    u32 data = 0;
    u32 recv = 0;
    
    self->write(self, HM2_SPI_DATA_REG, &data, sizeof(data));
    wait_for_data(self);
    self->read(self, HM2_SPI_DATA_REG, &recv, sizeof(recv));
    return (u8) recv & 0xFF;
}

void open_spi_access_hm2(spi_eeprom_dev_t *access) {
    access->set_cs_low = &hm2_set_cs_low, 
    access->set_cs_high = &hm2_set_cs_high;
    access->prefix = &hm2_prefix;
    access->suffix = &hm2_suffix;
    access->send_byte = &hm2_send_byte;
    access->recv_byte = &hm2_recv_byte;
};
