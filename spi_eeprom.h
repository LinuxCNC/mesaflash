
#ifndef __SPI_EEPROM_H
#define __SPI_EEPROM_H

#include "hostmot2.h"

#define DAV_MASK            0x04

typedef struct {
    void (*set_cs_low)(llio_t *self);
    void (*set_cs_high)(llio_t *self);
    void (*prefix)(llio_t *self);
    void (*suffix)(llio_t *self);
    void (*send_byte)(llio_t *self, u8 byte);
    u8   (*recv_byte)(llio_t *self);
} spi_eeprom_dev_t;

void open_spi_access_hm2(spi_eeprom_dev_t *access);

#endif

