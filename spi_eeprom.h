#ifndef __SPI_EEPROM_H
#define __SPI_EEPROM_H

#include "anyio.h"
#include "hostmot2.h"

#define ID_EEPROM_1M 0x10
#define ID_EEPROM_2M 0x11
#define ID_EEPROM_4M 0x12
#define ID_EEPROM_8M 0x13
#define ID_EEPROM_16M 0x14

#define PAGE_SIZE   256
#define SECTOR_SIZE 65536
#define BOOT_BLOCK_SIZE 64

#define BOOT_ADDRESS     0x000000
#define FALLBACK_ADDRESS 0x010000

#define SPI_CMD_PAGE_WRITE   0x02
#define SPI_CMD_READ         0x03
#define SPI_CMD_READ_STATUS  0x05
#define SPI_CMD_WRITE_ENABLE 0x06
#define SPI_CMD_READ_IDROM   0xAB
#define SPI_CMD_SECTOR_ERASE 0xD8

#define WRITE_IN_PROGRESS_MASK 0x01

u8 boot_block[BOOT_BLOCK_SIZE];

typedef struct {
    void (*set_cs_low)(llio_t *self);
    void (*set_cs_high)(llio_t *self);
    void (*prefix)(llio_t *self);
    void (*suffix)(llio_t *self);
    void (*send_byte)(llio_t *self, u8 byte);
    u8   (*recv_byte)(llio_t *self);
} spi_eeprom_dev_t;

char *eeprom_get_flash_type(u8 flash_id);
u32 eeprom_calc_user_space(u8 flash_id);
void prepare_boot_block(u8 flash_id);
int eeprom_write_area(llio_t *self, char *bitfile_name, u32 start_address);
int eeprom_verify_area(llio_t *self, char *bitfile_name, u32 start_address);
u8 read_flash_id(llio_t *self);
void eeprom_init(llio_t *self);
void eeprom_cleanup(llio_t *self);

#endif
