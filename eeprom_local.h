
#ifndef __EEPROM_LOCAL_H
#define __EEPROM_LOCAL_H

#include "hostmot2.h"

// gpio access
#define XIO2001_SBAD_STAT_REG 0x00B2
#define XIO2001_GPIO_ADDR_REG 0x00B4
#define XIO2001_GPIO_DATA_REG 0x00B6

#define DATA_MASK 0x80
#define ADDR_MASK 0x800000

#define CMD_LEN  8
#define DATA_LEN 8
#define ADDR_LEN 24

// io access
#define IO_DAV_MASK           0x04
#define SPI_SREG_OFFSET       0x40
#define SPI_CS_OFFSET         0x44

typedef struct {
    void (*set_cs_low)(llio_t *self);
    void (*set_cs_high)(llio_t *self);
    void (*prefix)(llio_t *self);
    void (*suffix)(llio_t *self);
    void (*send_byte)(llio_t *self, u8 byte);
    u8   (*recv_byte)(llio_t *self);
} spi_eeprom_dev_t;

u8 read_flash_id(llio_t *self);
int local_write_flash(llio_t *self, char *bitfile_name, u32 start_address);
int local_verify_flash(llio_t *self, char *bitfile_name, u32 start_address);
void open_spi_access_local(llio_t *self);
void close_spi_access_local(llio_t *self);

#endif


