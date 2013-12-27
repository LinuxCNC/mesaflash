
#ifndef __SPI_ACCESS_IO_H
#define __SPI_ACCESS_IO_H

#include "spi_eeprom.h"
#include "hostmot2.h"

#define DAV_MASK            0x04
#define SPI_SREG_OFFSET     0x40
#define SPI_CS_OFFSET       0x44

void open_spi_access_io(llio_t *self, spi_eeprom_dev_t *access);
void close_spi_access_io(llio_t *self, spi_eeprom_dev_t *access);

#endif


