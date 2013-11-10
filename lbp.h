
#ifndef __LBP_H
#define __LBP_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "common.h"

#define LBP_SENDRECV_DEBUG 0

#define LBP16_ARGS_8BIT         00000000b
#define LBP16_ARGS_16BIT        00000001b
#define LBP16_ARGS_32BIT        00000010b
#define LBP16_ARGS_64BIT        00000011b
#define LBP16_ADDR              00000100b
#define LBP16_NO_ADDR           00000000b
#define LBP_ADDR_AUTO_INC       00001000b
#define LBP_RPC_INCLUDES_DATA   00010000b
#define LBP_WRITE               00100000b
#define LBP_READ                00000000b
#define LBP_CMD_RW              01000000b

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

u8 lbp_read_ctrl(u8 cmd);
void lbp_init(board_access_t *access);
void lbp_release();

#endif

