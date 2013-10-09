
#ifndef __SPI_ACCESS_GPIO_H
#define __SPI_ACCESS_GPIO_H

#include "spi_eeprom.h"
#include "hostmot2.h"

#define XIO2001_SBAD_STAT_REG 0x00B2
#define XIO2001_GPIO_ADDR_REG 0x00B4
#define XIO2001_GPIO_DATA_REG 0x00B6

#define DATA_MASK 0x80
#define ADDR_MASK 0x800000

#define CMD_LEN  8
#define DATA_LEN 8
#define ADDR_LEN 24

void open_spi_access_gpio(llio_t *self, spi_eeprom_dev_t *access);
void close_spi_access_gpio(llio_t *self, spi_eeprom_dev_t *access);

#endif
