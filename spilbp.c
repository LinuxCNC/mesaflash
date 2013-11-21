
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "anyio.h"

void spilbp_print_info() {
}

void spilbp_init(board_access_t *access) {
}

void spilbp_release() {
}
