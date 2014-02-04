
#ifndef __EEPROM_LOCAL_H
#define __EEPROM_LOCAL_H

#include "hostmot2.h"

// hm2 access
#define HM2_DAV_MASK          0x04

#define SPI_CMD_PAGE_WRITE   0x02
#define SPI_CMD_READ         0x03
#define SPI_CMD_READ_STATUS  0x05
#define SPI_CMD_WRITE_ENABLE 0x06
#define SPI_CMD_READ_IDROM   0xAB
#define SPI_CMD_SECTOR_ERASE 0xD8

#define WRITE_IN_PROGRESS_MASK 0x01

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

int local_write_flash(llio_t *self, char *bitfile_name, u32 start_address);
int local_verify_flash(llio_t *self, char *bitfile_name, u32 start_address);
void open_spi_access_local(llio_t *self);
void close_spi_access_local(llio_t *self);

#endif


