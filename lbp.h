
#ifndef __LBP_H
#define __LBP_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "common.h"

#define LBP_SENDRECV_DEBUG 0

#define LBP_COOKIE              0x5A

#define LBP_ARGS_8BIT           0b00000000
#define LBP_ARGS_16BIT          0b00000001
#define LBP_ARGS_32BIT          0b00000010
#define LBP_ARGS_64BIT          0b00000011
#define LBP_ADDR                0b00000100
#define LBP_NO_ADDR             0b00000000
#define LBP_ADDR_AUTO_INC       0b00001000
#define LBP_RPC_INCLUDES_DATA   0b00010000
#define LBP_WRITE               0b00100000
#define LBP_READ                0b00000000
#define LBP_CMD_RW              0b01000000

#define LBP_CMD_READ            (LBP_CMD_RW | LBP_READ | LBP_ADDR)
#define LBP_CMD_WRITE           (LBP_CMD_RW | LBP_WRITE | LBP_ADDR)

#define LBP_CMD_READ_UNIT_ADDR      0xC0
#define LBP_CMD_READ_STATUS         0xC1
#define LBP_CMD_READ_ENA_RPCMEM     0xCA
#define LBP_CMD_READ_CMD_TIMEOUT    0xCB
#define LBP_CMD_READ_DEV_NAME0      0xD0
#define LBP_CMD_READ_DEV_NAME1      0xD1
#define LBP_CMD_READ_DEV_NAME2      0xD2
#define LBP_CMD_READ_DEV_NAME3      0xD3
#define LBP_CMD_READ_CONFIG_NAME0   0xD4
#define LBP_CMD_READ_CONFIG_NAME1   0xD5
#define LBP_CMD_READ_CONFIG_NAME2   0xD6
#define LBP_CMD_READ_CONFIG_NAME3   0xD7
#define LBP_CMD_READ_LO_ADDR        0xD8
#define LBP_CMD_READ_HI_ADDR        0xD9
#define LBP_CMD_READ_VERSION        0xDA
#define LBP_CMD_READ_UNIT_ID        0xDB
#define LBP_CMD_READ_RPC_PITCH      0xDC
#define LBP_CMD_READ_RPC_SIZE_LO    0xDD
#define LBP_CMD_READ_RPC_SIZE_HI    0xDE
#define LBP_CMD_READ_COOKIE         0xDF

struct lbp_cmd_addr_struct {
    u8 cmd;
    u8 addr_hi;
    u8 addr_lo;
} __attribute__ ((__packed__));
typedef struct lbp_cmd_addr_struct lbp_cmd_addr;

struct lbp_cmd_addr_data_struct {
    u8 cmd;
    u8 addr_hi;
    u8 addr_lo;
    u32 data;
} __attribute__ ((__packed__));
typedef struct lbp_cmd_addr_data_struct lbp_cmd_addr_data;

int lbp_send(void *packet, int size);
int lbp_recv(void *packet, int size);
u8 lbp_read_ctrl(u8 cmd);
int lbp_read(u16 addr, void *buffer);
int lbp_write(u16 addr, void *buffer);
void lbp_print_info();
void lbp_init(board_access_t *access);
void lbp_release();

#endif

