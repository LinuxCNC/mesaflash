
#ifndef __HOSTMOT2_H
#define __HOSTMOT2_H

#include <pci/pci.h>

#define HM2_AREA_SIZE    0x10000

#define HM2_SPI_CTRL_REG 0x0070
#define HM2_SPI_DATA_REG 0x0074
#define HM2_COOKIE_REG   0x0100
#define HM2_CONFIG_NAME  0x0104
#define HM2_IDROM_ADDR   0x010C

#define HM2_COOKIE       0x55AACAFE

#define HM2_MAX_MODULES  32
#define HM2_MAX_PINS     144
#define HM2_MAX_TAGS     28

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
    int num_ioport_connectors;
    int pins_per_connector;
    const char *ioport_connector_name[ANYIO_MAX_IOPORT_CONNECTORS];
    int num_leds;
    const char *fpga_part_number;
    char board_name[16];
    void *private;
    hostmot2_t hm2;
};

void hm2_print_idrom(hostmot2_t *hm2);
void hm2_print_modules(hostmot2_t *hm2);
void hm2_print_pins(hostmot2_t *hm2);

#endif

