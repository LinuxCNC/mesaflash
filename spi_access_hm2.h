
#ifndef __SPI_ACCESS_HM2_H
#define __SPI_ACCESS_HM2_H

#include "spi_eeprom.h"
#include "hostmot2.h"

#define DAV_MASK            0x04

void open_spi_access_hm2(llio_t *self, spi_eeprom_dev_t *access);
void close_spi_access_hm2(llio_t *self, spi_eeprom_dev_t *access);

#endif

