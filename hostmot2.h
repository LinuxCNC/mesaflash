
#ifndef __HOSTMOT2_H
#define __HOSTMOT2_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#define HM2_AREA_SIZE    0x10000

#define HM2_SPI_CTRL_REG 0x0070
#define HM2_SPI_DATA_REG 0x0074
#define HM2_COOKIE_REG   0x0100
#define HM2_CONFIG_NAME  0x0104
#define HM2_IDROM_ADDR   0x010C

#define HM2_CONFIG_NAME_LEN 8
#define HM2_COOKIE          0x55AACAFE

#define HM2_MODULE_LED                      0x0200
#define HM2_MODULE_WD_TIMER                 0x0C00
#define HM2_MODULE_WD_STATUS                0x0D00
#define HM2_MODULE_WD_RESET                 0x0E00
#define HM2_MODULE_GPIO                     0x1000
#define HM2_MODULE_GPIO_DDR                 0x1100
#define HM2_MODULE_GPIO_ALT_SOURCE          0x1200
#define HM2_MODULE_GPIO_OPEN_DRAIN          0x1300
#define HM2_MODULE_GPIO_INVERT_OUT          0x1400
#define HM2_MODULE_STEPGEN_RATE             0x2000
#define HM2_MODULE_STEPGEN_ACCUM            0x2100
#define HM2_MODULE_STEPGEN_MODE             0x2200
#define HM2_MODULE_STEPGEN_DIR_SETUP        0x2300
#define HM2_MODULE_STEPGEN_DIR_HOLD         0x2400
#define HM2_MODULE_STEPGEN_STEP_LEN         0x2500
#define HM2_MODULE_STEPGEN_STEP_SPACE       0x2600
#define HM2_MODULE_STEPGEN_TABLE_DATA       0x2700
#define HM2_MODULE_STEPGEN_TABLE_LEN        0x2800
#define HM2_MODULE_STEPGEN_DDS              0x2900
#define HM2_MODULE_WAVEGEN_RATE             0x2A00
#define HM2_MODULE_WAVEGEN_PDM              0x2B00
#define HM2_MODULE_WAVEGEN_TABLE_LEN        0x2C00
#define HM2_MODULE_WAVEGEN_TABLE_PTR        0x2D00
#define HM2_MODULE_WAVEGEN_TABLE_DATA       0x2E00
#define HM2_MODULE_ENCODER_COUNTER          0x3000
#define HM2_MODULE_ENCODER_COUNTER_CCR      0x3100
#define HM2_MODULE_ENCODER_TSSDIV           0x3200
#define HM2_MODULE_ENCODER_TS_COUNT         0x3300
#define HM2_MODULE_ENCODER_FILTER_RATE      0x3400

/// SSERIAL MODULE

#define HM2_MODULE_SSERIAL                  0x5A00
#define HM2_MODULE_SSERIAL_CMD              HM2_MODULE_SSERIAL
#define HM2_MODULE_SSERIAL_DATA             (HM2_MODULE_SSERIAL + 0x100)
#define HM2_MODULE_SSERIAL_CS               (HM2_MODULE_SSERIAL + 0x200)
#define HM2_MODULE_SSERIAL_INTERFACE0       (HM2_MODULE_SSERIAL + 0x300)
#define HM2_MODULE_SSERIAL_INTERFACE1       (HM2_MODULE_SSERIAL + 0x400)
#define HM2_MODULE_SSERIAL_INTERFACE2       (HM2_MODULE_SSERIAL + 0x500)
#define HM2_MAX_SSERIAL_MODULES  2
#define HM2_MAX_SSERIAL_CHANNELS 16

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
#define SSLBP_CMD_READ(x)              (SSLBP_REQUEST | SSLBP_READ | (x & 0xFF))
//#define SSLBP_CMD_WRITE(x)             (SSLBP_REQUEST | SSLBP_WRITE | ((x & 0xFF))

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

typedef struct {
    u32 unit;
    char name[4];
} sserial_device_t;

#define HM2_MAX_MODULES  32
#define HM2_MAX_PINS     144
#define HM2_MAX_TAGS     28
#define HM2_MAX_CHANNELS 64

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
#define HM2_GTAG_LIOPORT           0x40
#define HM2_GTAG_LED               0x80
#define HM2_GTAG_RESOLVER          0xC0
#define HM2_GTAG_SSERIAL           0xC1
#define HM2_GTAG_TWIDDLER          0xC2

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

#define ANYIO_MAX_IOPORT_CONNECTORS 8

typedef struct llio_struct llio_t;

typedef struct {
    llio_t *board;
    char config_name[8];
    hm2_idrom_desc_t idrom;
    hm2_module_desc_t modules[HM2_MAX_MODULES];
    hm2_pin_desc_t pins[HM2_MAX_PINS];
} hostmot2_t;

struct llio_struct {
    int (*read)(llio_t *self, u32 addr, void *buffer, int size);
    int (*write)(llio_t *self, u32 addr, void *buffer, int size);
    int (*program_flash)(llio_t *self, char *bitfile_name, u32 start_address);
    int (*verify_flash)(llio_t *self, char *bitfile_name, u32 start_address);
    int (*program_fpga)(llio_t *self, char *bitfile_name);
    int (*reset)(llio_t *self);
    int num_ioport_connectors;
    int pins_per_connector;
    const char *ioport_connector_name[ANYIO_MAX_IOPORT_CONNECTORS];
    int num_leds;
    const char *fpga_part_number;
    char board_name[16];
    void *private;
    hostmot2_t hm2;
    sserial_interface_t ss_interface[HM2_MAX_SSERIAL_MODULES];
    sserial_device_t ss_device[HM2_MAX_SSERIAL_CHANNELS];
    int verbose;
};

typedef struct {
    u8 tag;
    char *name[10];
} pin_name_t;

typedef struct {
    char *name;
    u8 tag;
} mod_name_t;

void hm2_read_idrom(llio_t *llio);
hm2_module_desc_t *hm2_find_module(hostmot2_t *hm2, u8 gtag);
void hm2_print_idrom(hostmot2_t *hm2);
void hm2_print_modules(hostmot2_t *hm2);
void hm2_print_pins(hostmot2_t *hm2);
void hm2_print_pin_file(llio_t *llio);
void sserial_module_init(llio_t *llio);

#endif

