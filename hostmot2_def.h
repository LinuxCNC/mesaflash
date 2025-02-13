//
//    Copyright (C) 2013-2014 Michael Geszkiewicz
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#ifndef __HOSTMOT2_DEF_H
#define __HOSTMOT2_DEF_H

#include "types.h"

#define HM2_AREA_SIZE    0x10000

// 
// Pin Descriptor constants
// 

#define HM2_PIN_SOURCE_IS_PRIMARY   0x00
#define HM2_PIN_SOURCE_IS_SECONDARY 0x01
#define HM2_PIN_DIR_IS_INPUT        0x02
#define HM2_PIN_DIR_IS_OUTPUT       0x04

#define HM2_GTAG_NONE              0x00
#define HM2_GTAG_IRQ_LOGIC         0x01
#define HM2_GTAG_WATCHDOG          0x02
#define HM2_GTAG_IOPORT            0x03
#define HM2_GTAG_ENCODER           0x04
#define HM2_GTAG_STEPGEN           0x05
#define HM2_GTAG_PWMGEN            0x06
#define HM2_GTAG_SPI               0x07
#define HM2_GTAG_SSI               0x08
#define HM2_GTAG_UART_TX           0x09
#define HM2_GTAG_UART_RX           0x0A
#define HM2_GTAG_TRAM              0x0B
#define HM2_GTAG_MUXED_ENCODER     0x0C
#define HM2_GTAG_MUXED_ENCODER_SEL 0x0D
#define HM2_GTAG_BSPI              0x0E
#define HM2_GTAG_DBSPI             0x0F
#define HM2_GTAG_DPLL              0x10
#define HM2_GTAG_MUXED_ENCODER_MIM 0x11
#define HM2_GTAG_MUXED_ENCODER_SEL_MIM 0x12
#define HM2_GTAG_TPPWM             0x13
#define HM2_GTAG_WAVEGEN           0x14
#define HM2_GTAG_DAQ_FIFO          0x15
#define HM2_GTAG_BIN_OSC           0x16
#define HM2_GTAG_BIN_DMDMA         0x17
#define HM2_GTAG_BISS              0x18
#define HM2_GTAG_FABS              0x19
#define HM2_GTAG_HM2DPLL           0x1A
#define HM2_GTAG_PKTUART_TX        0x1B
#define HM2_GTAG_PKTUART_RX        0x1C
#define HM2_GTAG_INMUX             0x1E
#define HM2_GTAG_SIGMA5            0x1F
#define HM2_GTAG_INM               0x23
#define HM2_GTAG_DPAINTER          0x2A
#define HM2_GTAG_XYMOD             0x2B
#define HM2_GTAG_RCPWMGEN          0x2C
#define HM2_GTAG_OUTM              0x2D
#define HM2_GTAG_LIOPORT           0x40
#define HM2_GTAG_LED               0x80
#define HM2_GTAG_RESOLVER          0xC0
#define HM2_GTAG_SSERIAL           0xC1
#define HM2_GTAG_TWIDDLER          0xC2
#define HM2_GTAG_SSR               0xC3
#define HM2_GTAG_CPDRIVE           0xC4
#define HM2_GTAG_DSAD              0xC5
#define HM2_GTAG_SSERIALB          0xC6
#define HM2_GTAG_ONESHOT           0xC7
#define HM2_GTAG_PERIOD            0xC8


// HM2 EEPROM SPI INTERFACE

#define HM2_SPI_CTRL_REG    0x0070
#define HM2_SPI_DATA_REG    0x0074

// HM2 ICAP INTERFACE

#define HM2_ICAP_REG        0x0078
#define HM2_ICAP_COOKIE     0x1CAB1CAB

// hm2 access
#define SPI_CMD_PAGE_WRITE      0x02
#define SPI_CMD_READ            0x03
#define SPI_CMD_READ_STATUS     0x05
#define SPI_CMD_WRITE_ENABLE    0x06
#define SPI_CMD_READ_IDROM      0xAB
#define SPI_CMD_SECTOR_ERASE    0xD8

#define WRITE_IN_PROGRESS_MASK  0x01

// IDROM

#define HM2_COOKIE_REG      0x0100
#define HM2_CONFIG_NAME     0x0104
#define HM2_IDROM_ADDR      0x010C

#define HM2_CONFIG_NAME_LEN 8
#define HM2_COOKIE          0x55AACAFE

#define HM2_MAX_MODULES     32
#define HM2_MAX_PINS        144

typedef struct {
    u32 idrom_type;
    u32 offset_to_modules;
    u32 offset_to_pins;
    char board_name[8];
    u32 fpga_size;
    u32 fpga_pins;
    u32 io_ports;
    u32 io_width;
    u32 port_width;
    u32 clock_low;
    u32 clock_high;
    u32 instance_stride0;
    u32 instance_stride1;
    u32 register_stride0;
    u32 register_stride1;
} hm2_idrom_desc_t;

#define HM2_CLOCK_LOW_TAG  0x01
#define HM2_CLOCK_HIGH_TAG 0x02

typedef struct {
    u8 gtag;
    u8 version;
    u8 clock_tag;
    u8 instances;
    u16 base_address;
    u8 registers;
    u8 strides;
    u32 mp_bitmap;
} hm2_module_desc_t;

#define HM2_CHAN_GLOBAL 0x80
#define HM2_PIN_OUTPUT  0x80

typedef struct {
    u8 sec_pin;
    u8 sec_tag;
    u8 sec_chan;
    u8 gtag;
} hm2_pin_desc_t;

// LEDS MODULE

#define HM2_MOD_OFFS_LED                      0x0200

// WATCHDOG MODULE

#define HM2_MOD_OFFS_WD_TIMER                 0x0000
#define HM2_MOD_OFFS_WD_STATUS                0x0100
#define HM2_MOD_OFFS_WD_RESET                 0x0200

// GPIO MODULE

#define HM2_MOD_OFFS_GPIO                     0x0000
#define HM2_MOD_OFFS_GPIO_DDR                 0x0100
#define HM2_MOD_OFFS_GPIO_ALT_SOURCE          0x0200
#define HM2_MOD_OFFS_GPIO_OPEN_DRAIN          0x0300
#define HM2_MOD_OFFS_GPIO_INVERT_OUT          0x0400

// STEPGEN MODULE

#define HM2_MOD_OFFS_STEPGEN_RATE             0x0000
#define HM2_MOD_OFFS_STEPGEN_ACCUM            0x0100
#define HM2_MOD_OFFS_STEPGEN_MODE             0x0200
#define HM2_MOD_OFFS_STEPGEN_DIR_SETUP        0x0300
#define HM2_MOD_OFFS_STEPGEN_DIR_HOLD         0x0400
#define HM2_MOD_OFFS_STEPGEN_STEP_LEN         0x0500
#define HM2_MOD_OFFS_STEPGEN_STEP_SPACE       0x0600
#define HM2_MOD_OFFS_STEPGEN_TABLE_DATA       0x0700
#define HM2_MOD_OFFS_STEPGEN_TABLE_LEN        0x0800
#define HM2_MOD_OFFS_STEPGEN_DDS              0x0900

// ENCODER MODULE

#define HM2_MOD_OFFS_ENCODER_COUNTER          0x0000
#define HM2_MOD_OFFS_ENCODER_LATCH_CCR        0x0100
#define HM2_MOD_OFFS_ENCODER_TSSDIV           0x0200
#define HM2_MOD_OFFS_ENCODER_TS_COUNT         0x0300
#define HM2_MOD_OFFS_ENCODER_FILTER_RATE      0x0400

// MUXED ENCODER MODULE

#define HM2_MOD_OFFS_MUX_ENCODER_COUNTER      0x0000
#define HM2_MOD_OFFS_MUX_ENCODER_LATCH_CCR    0x0100
#define HM2_MOD_OFFS_MUX_ENCODER_TSSDIV       0x0200
#define HM2_MOD_OFFS_MUX_ENCODER_TS_COUNT     0x0300
#define HM2_MOD_OFFS_MUX_ENCODER_FILTER_RATE  0x0400

#define HM2_ENCODER_QUADRATURE_ERROR    (1 << 15)
#define HM2_ENCODER_AB_MASK_POLARITY    (1 << 14)
#define HM2_ENCODER_LATCH_ON_PROBE      (1 << 13)
#define HM2_ENCODER_PROBE_POLARITY      (1 << 12)
#define HM2_ENCODER_FILTER              (1 << 11)
#define HM2_ENCODER_COUNTER_MODE        (1 << 10)
#define HM2_ENCODER_INDEX_MASK          (1 << 9)
#define HM2_ENCODER_INDEX_MASK_POLARITY (1 << 8)
#define HM2_ENCODER_INDEX_JUSTONCE      (1 << 6)
#define HM2_ENCODER_CLEAR_ON_INDEX      (1 << 5)
#define HM2_ENCODER_LATCH_ON_INDEX      (1 << 4)
#define HM2_ENCODER_INDEX_POLARITY      (1 << 3)
#define HM2_ENCODER_INPUT_INDEX         (1 << 2)
#define HM2_ENCODER_INPUT_B             (1 << 1)
#define HM2_ENCODER_INPUT_A             (1 << 0)

#define HM2_ENCODER_CONTROL_MASK        0x0000FFFF

// SSERIAL MODULE

#define HM2_SSERIAL_MAX_INTERFACES      4
#define HM2_SSERIAL_MAX_CHANNELS        8

#define HM2_MOD_OFFS_SSERIAL_CMD              0x0000
#define HM2_MOD_OFFS_SSERIAL_DATA             0x0100
#define HM2_MOD_OFFS_SSERIAL_CS               0x0200
#define HM2_MOD_OFFS_SSERIAL_INTERFACE0       0x0300
#define HM2_MOD_OFFS_SSERIAL_INTERFACE1       0x0400
#define HM2_MOD_OFFS_SSERIAL_INTERFACE2       0x0500

#define SSLBP_TYPE_LOC              0
#define SSLBP_WIDTH_LOC             1
#define SSLBP_MAJOR_REV_LOC         2
#define SSLBP_MINOR_REV_LOC         3
#define SSLBP_CHANNEL_START_LOC     4
#define SSLBP_CHANNEL_STRIDE_LOC    5
#define SSLBP_PROCESSOR_TYPE_LOC    6
#define SSLBP_NR_CHANNELS_LOC       7
#define SSLBP_CLOCK_LOC             0x230

#define SSLBP_START_NORMAL  0x0900
#define SSLBP_START_SETUP   0x0F00
#define SSLBP_STOP          0x0800
#define SSLBP_DOIT          0x1000
#define SSLBP_REQUEST       0x2000
#define SSLBP_RESET         0x4000
#define SSLBP_READ          0x0000
#define SSLBP_WRITE         0x8000

#define SSLBP_CMD_START_NORMAL_MODE(x) (SSLBP_START_NORMAL | ((1 << x) & 0xFF))
#define SSLBP_CMD_START_SETUP_MODE(x)  (SSLBP_START_SETUP | ((1 << x) & 0xFF))
#define SSLBP_CMD_STOP(x)              (SSLBP_STOP | ((1 << x) & 0xFF))
#define SSLBP_CMD_STOPALL              (SSLBP_STOP)
#define SSLBP_CMD_RESET                (SSLBP_RESET)
#define SSLBP_CMD_DOIT(x)              (SSLBP_DOIT | ((1 << x) & 0xFF))
#define SSLBP_CMD_READ(x)              (SSLBP_REQUEST | SSLBP_READ | (x & 0x3FF))
#define SSLBP_CMD_WRITE(x)             (SSLBP_REQUEST | SSLBP_WRITE | ((x & 0x3FF))

#define LBP_DATA                0xA0
#define LBP_MODE                0xB0

#define LBP_IN                  0x00
#define LBP_IO                  0x40
#define LBP_OUT                 0x80

#define LBP_PAD                 0x00
#define LBP_BITS                0x01
#define LBP_UNSIGNED            0x02
#define LBP_SIGNED              0x03
#define LBP_NONVOL_UNSIGNED     0x04
#define LBP_NONVOL_SIGNED       0x05
#define LBP_STREAM              0x06
#define LBP_BOOLEAN             0x07
#define LBP_ENCODER             0x08
#define LBP_ENCODER_H           0x18 // For Fanuc Absolute Encoders with separate
#define LBP_ENCODER_L           0x28 // part and full count fields

#define SSLBP_REMOTE_7I76_IO_SPINDLE 0x10000000
#define SSLBP_REMOTE_7I77_ANALOG     0x11000000
#define SSLBP_REMOTE_7I77_IO         0x12000000

// must match BOB name order
# define BOB_7I76 1
# define BOB_7I77 2
# define BOB_7I94_0 3
# define BOB_7I94_1 4
# define BOB_7I95_0 5
# define BOB_7I95_1 6
# define BOB_7I96_0 7
# define BOB_7I96_1 8
# define BOB_7I96_2 9
# define BOB_7I97_0 10
# define BOB_7I97_1 11
# define BOB_7I97_2 12
# define BOB_7C80_0 13
# define BOB_7C80_1 14
# define BOB_7C81_0 15
# define BOB_7C81_1 16
# define BOB_7C81_2 17
# define BOB_7I74 18
# define BOB_7I78 19
# define BOB_7I85 20
# define BOB_7I85S 21
# define BOB_7I88 22
# define BOB_7I89 23
# define BOB_DMM4250 24
# define BOB_5ABOB 25
# define BOB_G540 26
# define BOB_C11 27
# define BOB_C11G 28
# define BOB_7I33TA 29
# define BOB_7I37TA 30
# define BOB_7I44 31
# define BOB_7I47 32
# define BOB_7I47S 33
# define BOB_7I48 34
# define BOB_7I52 35
# define BOB_7I52S 36
# define BOB_MX3660 37
# define BOB_MX4660_1 38
# define BOB_MX4660_2 39
# define BOB_7I75 40
# define BOB_BENEZAN 41
# define BOB_HDR26 42




struct sserial_pdd_struct {
    u8 record_type;
    u8 data_size;
    u8 data_type;
    u8 data_dir;
    float param_min;
    float param_max;
    u16 param_addr;
} __attribute__ ((__packed__));
typedef struct sserial_pdd_struct sserial_pdd_t;

struct sserial_md_struct {
    u8 record_type;
    u8 mode_index;
    u8 mode_type;
    u8 unused;
};
typedef struct sserial_md_struct sserial_md_t;

typedef struct {
    u8 type;
    u8 width;
    u8 ver_major;
    u8 ver_minor;
    u8 gp_inputs;
    u8 gp_outputs;
    u8 processor_type;
    u8 channels_count;
    u32 clock;
    int baud_rate;
} sserial_interface_t;

#endif
