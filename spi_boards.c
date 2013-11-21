
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "anyio.h"
#include "spi_boards.h"
#include "common.h"
#include "spilbp.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;

int spi_boards_init(board_access_t *access) {
    spilbp_init(access);
    return 0;
}

void spi_boards_scan(board_access_t *access) {
}

void spi_boards_release(board_access_t *access) {
    spilbp_release();
}

void spi_print_info(board_t *board) {
}

