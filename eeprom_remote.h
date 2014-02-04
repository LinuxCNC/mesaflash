
#ifndef __EEPROM_REMOTE_H
#define __EEPROM_REMOTE_H

#include "hostmot2.h"

int remote_write_flash(llio_t *self, char *bitfile_name, u32 start_address);
int remote_verify_flash(llio_t *self, char *bitfile_name, u32 start_address);
void open_spi_access_remote(llio_t *self);
void close_spi_access_remote(llio_t *self);

#endif


