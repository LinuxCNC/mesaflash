#ifndef __ANYIO_H
#define __ANYIO_H

#ifdef _WIN32
#include <windef.h>
#include "winio32/winio.h"
#endif
#include "hostmot2.h"

#define MAX_BOARDS 8

typedef enum {BOARD_ETH, BOARD_PCI, BOARD_EPP, BOARD_USB, BOARD_SPI} board_type;
typedef enum {BOARD_MODE_CPLD, BOARD_MODE_FPGA} board_mode;
typedef enum {BOARD_FLASH_NONE, BOARD_FLASH_HM2, BOARD_FLASH_IO, BOARD_FLASH_GPIO, BOARD_FLASH_REMOTE} board_flash;

typedef struct {
    board_type type;
    board_mode mode;
    board_flash flash;
    u8 flash_id;
    u32 flash_start_address;
    int fallback_support;
    char dev_addr[16];

    struct pci_dev *dev;
    void *base;
    int len;
#ifdef _WIN32
    tagPhysStruct_t mem_handle;
#endif
    u16 ctrl_base_addr;
    u16 data_base_addr;

    u16 base_lo;
    u16 base_hi;
    void *region;
    void *region_hi;
    int epp_wide;

    llio_t llio;
} board_t;

typedef struct {
    char *device_name;
    int verbose;
    int recover;
    int pci;
    int epp;
    int usb;
    int eth;
    int spi;
    char *dev_addr;
    u16 epp_base_addr;
    u16 epp_base_hi_addr;
} board_access_t;

board_t boards[MAX_BOARDS];
int boards_count;

int anyio_init(board_access_t *access);
void anyio_cleanup(board_access_t *access);
void anyio_scan(board_access_t *access);
board_t *anyio_get_dev(board_access_t *access);
void anyio_set_active_dev(board_t *board);
void anyio_dev_print_info(board_t *board);
void anyio_dev_print_hm2_info(board_t *board);
void anyio_dev_print_sserial_info(board_t *board);
void anyio_bitfile_print_info(char *bitfile_name);

#endif
