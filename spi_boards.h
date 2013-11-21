
#ifndef __SPI_BOARDS_H
#define __SPI_BOARDS_H

#include "anyio.h"

int spi_boards_init(board_access_t *access);
void spi_boards_scan(board_access_t *access);
void spi_boards_release(board_access_t *access);
void spi_print_info(board_t *board);

#endif

