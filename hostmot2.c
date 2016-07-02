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

#include <stdio.h>
#include <string.h>
#include "hostmot2.h"

void hm2_read_idrom(hostmot2_t *hm2) {
    u32 idrom_addr, cookie;
    int i;

    hm2->llio->read(hm2->llio, HM2_COOKIE_REG, &(cookie), sizeof(u32));
    if (cookie != HM2_COOKIE) {
        printf("ERROR: no HOSTMOT2 firmware found. %X\n", cookie);
        return;
    }
    // check if it was already readed
    if (strncmp(hm2->config_name, "HOSTMOT2", 8) == 0)
        return;

    hm2->llio->read(hm2->llio, HM2_CONFIG_NAME, &(hm2->config_name), HM2_CONFIG_NAME_LEN);
    hm2->llio->read(hm2->llio, HM2_IDROM_ADDR, &(idrom_addr), sizeof(u32));
    hm2->llio->read(hm2->llio, idrom_addr, &(hm2->idrom), sizeof(hm2->idrom));
    for (i = 0; i < HM2_MAX_MODULES; i++) {
        hm2->llio->read(hm2->llio, idrom_addr + hm2->idrom.offset_to_modules + i*sizeof(hm2_module_desc_t),
          &(hm2->modules[i]), sizeof(hm2_module_desc_t));
    }
    for (i = 0; i < HM2_MAX_PINS; i++) {
        hm2->llio->read(hm2->llio, idrom_addr + hm2->idrom.offset_to_pins + i*sizeof(hm2_pin_desc_t),
          &(hm2->pins[i]), sizeof(hm2_module_desc_t));
    }
}

const char *hm2_hz_to_mhz(u32 freq_hz) {
    static char mhz_str[20];
    int r;
    int freq_mhz, freq_mhz_fractional;

    freq_mhz = freq_hz / (1000*1000);
    freq_mhz_fractional = (freq_hz / 1000) % 1000;
    r = snprintf(mhz_str, sizeof(mhz_str), "%d.%03d", freq_mhz, freq_mhz_fractional);
    if (r >= sizeof(mhz_str)) {
        printf("too many MHz!\n");
        return "(unpresentable)";
    }

    return mhz_str;
}

const char *hm2_get_general_function_name(int gtag) {
    switch (gtag) {
        case HM2_GTAG_IRQ_LOGIC:         return "IRQ logic";
        case HM2_GTAG_WATCHDOG:          return "Watchdog";
        case HM2_GTAG_IOPORT:            return "IOPort";
        case HM2_GTAG_ENCODER:           return "Encoder";
        case HM2_GTAG_STEPGEN:           return "StepGen";
        case HM2_GTAG_PWMGEN:            return "PWMGen";
        case HM2_GTAG_SPI:               return "SPI";
        case HM2_GTAG_SSI:               return "SSI";
        case HM2_GTAG_UART_TX:           return "UART Transmit Channel";
        case HM2_GTAG_UART_RX:           return "UART Receive Channel";
        case HM2_GTAG_TRAM:              return "TrnslationRAM";
        case HM2_GTAG_MUXED_ENCODER:     return "Muxed Encoder";
        case HM2_GTAG_MUXED_ENCODER_SEL: return "Muxed Encoder Select";
        case HM2_GTAG_BSPI:              return "Buffered SPI Interface";
        case HM2_GTAG_DBSPI:             return "DBSPI";
        case HM2_GTAG_DPLL:              return "DPLL";
        case HM2_GTAG_MUXED_ENCODER_MIM: return "Muxed Encoder MIM";
        case HM2_GTAG_MUXED_ENCODER_SEL_MIM: return "Muxed Encoder Select MIM";
        case HM2_GTAG_TPPWM:             return "ThreePhasePWM";
        case HM2_GTAG_WAVEGEN:           return "WaveGen";
        case HM2_GTAG_DAQ_FIFO:          return "DAQ FIFO";
        case HM2_GTAG_BIN_OSC:           return "BIN OSC";
        case HM2_GTAG_BIN_DMDMA:         return "BIN DMDMA";
        case HM2_GTAG_BISS:              return "BISS";
        case HM2_GTAG_FABS:              return "FABS";
        case HM2_GTAG_HM2DPLL:           return "HM2DPLL";
        case HM2_GTAG_LIOPORT:           return "LIOPORT";
        case HM2_GTAG_LED:               return "LED";
        case HM2_GTAG_RESOLVER:          return "Resolver";
        case HM2_GTAG_SSERIAL:           return "Smart Serial Interface";
        case HM2_GTAG_TWIDDLER:          return "TWIDDLER";
        default: {
            static char unknown[100];
            snprintf(unknown, 100, "(unknown-gtag-%d)", gtag);
            printf("Firmware contains unknown function (gtag-%d)/n", gtag);
            return unknown;
        }
    }
}

static const char* hm2_get_pin_secondary_name(hm2_pin_desc_t *pin) {
    static char unknown[100];
    int sec_pin = pin->sec_pin & 0x7F;  // turn off the "pin is an output" bit

    switch (pin->sec_tag) {

        case HM2_GTAG_MUXED_ENCODER:
            switch (sec_pin) {
                case 1: return "Muxed A";
                case 2: return "Muxed B";
                case 3: return "Muxed Index";
                case 4: return "Muxed IndexMask";
            }
            break;

        case HM2_GTAG_MUXED_ENCODER_SEL:
            switch (sec_pin) {
                case 1: return "Mux Select 0";
                case 2: return "Mux Select 1";
            }
            break;

        case HM2_GTAG_ENCODER:
            switch (sec_pin) {
                case 1: return "A";
                case 2: return "B";
                case 3: return "Index";
                case 4: return "IndexMask";
                case 5: return "Probe";
            }
            break;
            
        case HM2_GTAG_RESOLVER:
            switch (sec_pin) {
                case 1: return "NC";
                case 2: return "REFPDM+";
                case 3: return "REFPDM-";
                case 4: return "AMUX0";
                case 5: return "AMUX1";
                case 6: return "AMUX2";
                case 7: return "SPICS";
                case 8: return "SPICLK";
                case 9: return "SPIDO0";
                case 10: return "SPIDO1";
            }
            break;

        case HM2_GTAG_PWMGEN:
            // FIXME: these depend on the pwmgen mode
            switch (sec_pin) {
                case 1: return "Out0 (PWM or Up)";
                case 2: return "Out1 (Dir or Down)";
                case 3: return "Not-Enable";
            }
            break;

        case HM2_GTAG_TPPWM:
            switch (sec_pin) {
                case 1: return "PWM A";
                case 2: return "PWM B";
                case 3: return "PWM C";
                case 4: return "PWM /A";
                case 5: return "PWM /B";
                case 6: return "PWM /C";
                case 7: return "Enable";
                case 8: return "Fault";
            }
            break;

        case HM2_GTAG_STEPGEN:
            // FIXME: these depend on the stepgen mode
            switch (sec_pin) {
                case 1: return "Step";
                case 2: return "Direction";
                case 3: return "(unused)";
                case 4: return "(unused)";
                case 5: return "(unused)";
                case 6: return "(unused)";
            }
            break;

        case HM2_GTAG_SSERIAL:
            if (pin->sec_pin & 0x80){ // Output pin codes
                switch (sec_pin) {
                    case 0x1: return "TxData0";
                    case 0x2: return "TxData1";
                    case 0x3: return "TxData2";
                    case 0x4: return "TxData3";
                    case 0x5: return "TxData4";
                    case 0x6: return "TxData5";
                    case 0x7: return "TxData6";
                    case 0x8: return "TxData7";
                    case 0x11: return "TxEn0  ";
                    case 0x12: return "TxEn1  ";
                    case 0x13: return "TxEn2  ";
                    case 0x14: return "TxEn3  ";
                    case 0x15: return "TxEn4  ";
                    case 0x16: return "TxEn5  ";
                    case 0x17: return "TxEn6  ";
                    case 0x18: return "TxEn7  ";
                }
                break;
            }else{ // INput Pin Codes
                switch (sec_pin) {
                    case 0x1: return "RxData0";
                    case 0x2: return "RxData1";
                    case 0x3: return "RxData2";
                    case 0x4: return "RxData3";
                    case 0x5: return "RxData4";
                    case 0x6: return "RxData5";
                    case 0x7: return "RxData6";
                    case 0x8: return "RxData7";
                }
                break;
            }
        case HM2_GTAG_BSPI:
            switch (sec_pin) {
                case 0x1: return "/Frame";
                case 0x2: return "Serial Out";
                case 0x3: return "Clock";
                case 0x4: return "Serial In";
                case 0x5: return "CS0";
                case 0x6: return "CS1";
                case 0x7: return "CS2";
                case 0x8: return "CS3";
                case 0x9: return "CS4";
                case 0xA: return "CS5";
                case 0xB: return "CS6";
                case 0xC: return "CS7";
            }
            break;

        case HM2_GTAG_UART_RX:
            switch (sec_pin) {
                case 0x1: return "RX Data";
            }
            break;
        case HM2_GTAG_UART_TX:    
            switch (sec_pin) {
                case 0x1: return "TX Data";
                case 0x2: return "Drv Enable";
            }
            break;

    }
    snprintf(unknown, sizeof(unknown), "unknown-pin-%d", sec_pin & 0x7F);
    return unknown;
}

hm2_module_desc_t *hm2_find_module(hostmot2_t *hm2, u8 gtag) {
    int i;

    for (i = 0; i < HM2_MAX_MODULES; i++) {
        if (hm2->modules[i].gtag == gtag)
            return &(hm2->modules[i]);
    }
    return NULL;
}

void hm2_set_pin_source(hostmot2_t *hm2, int pin_number, u8 source) {
    u32 data;
    u16 addr;
    hm2_module_desc_t *md = hm2_find_module(hm2, HM2_GTAG_IOPORT);
    
    if (md == NULL) {
        printf("hm2_set_pin_source(): no IOPORT module found\n");
        return;
    }
    if ((pin_number < 0) || (pin_number >= (hm2->idrom.io_ports*hm2->idrom.io_width))) {
        printf("hm2_set_pin_source(): invalid pin number %d\n", pin_number);
        return;
    }

    addr = md->base_address;
    hm2->llio->read(hm2->llio, addr + HM2_MOD_OFFS_GPIO_ALT_SOURCE + (pin_number / 24)*4, &data, sizeof(data));
    if (source == HM2_PIN_SOURCE_IS_PRIMARY) {
        data &= ~(1 << (pin_number % 24));
    } else if (source == HM2_PIN_SOURCE_IS_SECONDARY) {
        data |= (1 << (pin_number % 24));
    } else {
        printf("hm2_set_pin_source(): invalid pin source 0x%02X\n", source);
        return;
    }
    hm2->llio->write(hm2->llio, addr + HM2_MOD_OFFS_GPIO_ALT_SOURCE + (pin_number / 24)*4, &data, sizeof(data));
}

void hm2_set_pin_direction(hostmot2_t *hm2, int pin_number, u8 direction) {
    u32 data;
    u16 addr;
    hm2_module_desc_t *md = hm2_find_module(hm2, HM2_GTAG_IOPORT);

    if (md == NULL) {
        printf("hm2_set_pin_direction(): no IOPORT module found\n");
        return;
    }
    if ((pin_number < 0) || (pin_number >= (hm2->idrom.io_ports*hm2->idrom.io_width))) {
        printf("hm2_set_pin_direction(): invalid pin number %d\n", pin_number);
        return;
    }

    addr = md->base_address;
    hm2->llio->read(hm2->llio, addr + HM2_MOD_OFFS_GPIO_DDR + (pin_number / 24)*4, &data, sizeof(data));
    if (direction == HM2_PIN_DIR_IS_INPUT) {
        data &= ~(1 << (pin_number % 24));
    } else if (direction == HM2_PIN_DIR_IS_OUTPUT) {
        data |= (1 << (pin_number % 24));
    } else {
        printf("hm2_set_pin_direction(): invalid pin direction 0x%02X\n", direction);
        return;
    }
    hm2->llio->write(hm2->llio, addr + HM2_MOD_OFFS_GPIO_DDR + (pin_number / 24)*4, &data, sizeof(data));
}

void hm2_print_idrom(hostmot2_t *hm2) {
    printf("IDRom:\n");
    printf("  IDRom Type: 0x%02X\n", hm2->idrom.idrom_type);
    printf("  Offset to Modules: 0x%08X\n", hm2->idrom.offset_to_modules); 
    printf("  Offset to Pin Description: 0x%08X\n", hm2->idrom.offset_to_pins); 
    printf("  Board Name: %.*s\n", (int)sizeof(hm2->config_name), hm2->idrom.board_name);
    printf("  FPGA Size: %u\n", hm2->idrom.fpga_size);
    printf("  FPGA Pins: %u\n", hm2->idrom.fpga_pins);
    printf("  Port Width: %u\n", hm2->idrom.port_width);
    printf("  IO Ports: %u\n", hm2->idrom.io_ports);
    printf("  IO Width: %u\n", hm2->idrom.io_width);
    printf("  Clock Low: %d Hz (%d KHz, %d MHz)\n", hm2->idrom.clock_low, (hm2->idrom.clock_low / 1000), (hm2->idrom.clock_low / (1000 * 1000)));
    printf("  Clock High: %d Hz (%d KHz, %d MHz)\n", hm2->idrom.clock_high, (hm2->idrom.clock_high / 1000), (hm2->idrom.clock_high / (1000 * 1000)));
    printf("  Instance Stride 0: 0x%08X\n", hm2->idrom.instance_stride0);
    printf("  Instance Stride 1: 0x%08X\n", hm2->idrom.instance_stride1);
    printf("  Register Stride 0: 0x%08X\n", hm2->idrom.register_stride0);
    printf("  Register Stride 1: 0x%08X\n", hm2->idrom.register_stride1);
}

void hm2_print_modules(hostmot2_t *hm2) {
    int i;

    for (i = 0; i < HM2_MAX_MODULES; i++) {
        hm2_module_desc_t *mod = &hm2->modules[i];
        u32 addr = HM2_IDROM_ADDR + hm2->idrom.offset_to_modules + i*sizeof(hm2_module_desc_t);
        u32 clock_freq;

        if (mod->gtag == HM2_GTAG_NONE) break;
        printf("Module Descriptor %d at 0x%04X:\n", i, addr);
        printf("  General Function Tag: %d (%s)\n", mod->gtag, hm2_get_general_function_name(mod->gtag));
        printf("  Version: %d\n", mod->version);
        if (mod->clock_tag == HM2_CLOCK_LOW_TAG) {
            clock_freq = hm2->idrom.clock_low;
        } else if (mod->clock_tag == HM2_CLOCK_HIGH_TAG) {
            clock_freq = hm2->idrom.clock_high;
        } else {
            printf("Module Descriptor %d (at 0x%04x) has invalid ClockTag %d\n", i, addr, mod->clock_tag);
            return;
        }
        printf("  Clock Tag: %d (%s MHz)\n", mod->clock_tag, hm2_hz_to_mhz(clock_freq));
        printf("  Instances: %d\n", mod->instances);
        printf("  Base Address: 0x%04X\n", mod->base_address);
        printf("  -- Num Registers: %d\n", mod->registers);
        if ((mod->strides & 0x0000000F) == 0) {
            printf("  Register Stride: 0x%08X\n", hm2->idrom.register_stride0);
        } else if ((mod->strides & 0x0000000F) == 1) {
            printf("  Register Stride: 0x%08X\n", hm2->idrom.register_stride1);
        } else {
            printf("Module Descriptor %d (at 0x%04x) has invalid RegisterStride %d\n", i, addr, mod->strides);
            return;
        }
        if (((mod->strides >> 4) & 0x0000000F) == 0) {
            printf("  -- Instance Stride: 0x%08X\n", hm2->idrom.instance_stride0);
        } else if (((mod->strides >> 4) & 0x0000000F) == 1) {
            printf("  -- Instance Stride: 0x%08X\n", hm2->idrom.instance_stride1);
        } else {
            printf("Module Descriptor %d (at 0x%04x) has invalid InstanceStride %d\n", i, addr, mod->strides);
            return;
        }
        printf("  -- Multiple Registers: 0x%08X\n", mod->mp_bitmap);
    }
}

void hm2_print_pins(hostmot2_t *hm2) {
    int i;

    for (i = 0; i < HM2_MAX_PINS; i++) {
        hm2_pin_desc_t *pin = &hm2->pins[i];

        if (pin->gtag == 0) break;
        printf("pin %d:\n", i);
        printf("  Primary Tag: 0x%02X (%s)\n", pin->gtag, hm2_get_general_function_name(pin->gtag));
        if (pin->sec_tag != 0) {
            printf("  Secondary Tag: 0x%02X (%s)\n", pin->sec_tag, hm2_get_general_function_name(pin->sec_tag));
            printf("  Secondary Unit: 0x%02X\n", pin->sec_chan);
            printf("  Secondary Pin: 0x%02X (%s, %s)\n", pin->sec_pin, hm2_get_pin_secondary_name(pin), ((pin->sec_pin & 0x80) ? "Output" : "Input"));
        }
    }
}

// PIN FILE GENERATING SUPPORT

static pin_name_t pin_names[HM2_MAX_TAGS] = {
  {HM2_GTAG_NONE,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_IRQ_LOGIC,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_WATCHDOG,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_IOPORT,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_ENCODER,  {"Quad-A", "Quad-B", "Quad-IDX", "Quad-IDXM", "Quad-Probe", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_MUXED_ENCODER,  {"MuxQ-A", "MuxQ-B", "MuxQ-IDX", "MuxQ-IDXM", "Quad-ProbeM", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_MUXED_ENCODER_SEL,  {"MuxSel0", "MuxSel1", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_MUXED_ENCODER_MIM,  {"MuxQ-A", "MuxQ-B", "MuxQ-IDX", "MuxQ-IDXM", "Quad-ProbeM", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_MUXED_ENCODER_SEL_MIM,  {"MuxSel0", "MuxSel1", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_STEPGEN,  {"Step/Table1", "Dir/Table2", "Table3", "Table4", "Table5", "Table6", "SGindex", "SGProbe", "Null9", "Null10"}},
  {HM2_GTAG_PWMGEN,      {"PWM", "Dir", "/Enable", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_TPPWM,    {"PWMA", "PWMB", "PWMC", "NPWMA", "NPWMB", "NPWMC", "/ENA", "FAULT", "Null9", "Null10"}},
  {HM2_GTAG_WAVEGEN,  {"PDMA", "PDMB", "Trig0", "Trig1", "Trig2", "Trig3", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_DAQ_FIFO,  {"Data", "Strobe", "Full", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_BIN_OSC,  {"OscOut", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_BIN_DMDMA,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_RESOLVER,  {"PwrEn", "PDMP", "PDMM", "ADChan0", "ADChan1", "ADChan2", "SPICS", "SPIClk", "SPIDI0", "SPIDI1"}},
  {HM2_GTAG_SSERIAL,  {"RXData", "TXData", "TXEn", "TestPin", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_TWIDDLER,  {"InBit", "IOBit", "OutBit", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_SPI,      {"/Frame", "DOut", "SClk", "DIn", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_BSPI,     {"/Frame", "DOut", "SClk", "DIn", "CS0", "CS1", "CS2", "CS3", "Null9", "Null10"}},
  {HM2_GTAG_DBSPI,    {"Null1", "DOut", "SClk", "DIn", "/CS-FRM0", "/CS-FRM1", "/CS-FRM2", "/CS-FRM3", "Null9", "Null10"}},
  {HM2_GTAG_DPLL,     {"Sync", "DDSMSB", "FOut", "PostOut", "SyncToggle", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_SSI,      {"SClk", "SClkEn", "Data", "DAv", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_UART_TX,   {"TXData", "TXEna", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_UART_RX,   {"RXData", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_TRAM,    {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_LED,      {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
};

static pin_name_t pin_names_xml[HM2_MAX_TAGS] = {
  {HM2_GTAG_NONE,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_IRQ_LOGIC,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_WATCHDOG,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_IOPORT,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_ENCODER,  {"Phase A", "Phase B", "Index", "Quad-IDXM", "Quad-Probe", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_MUXED_ENCODER,  {"Muxed Phase A", "Muxed Phase B", "Muxed Index", "MuxQ-IDXM", "Quad-ProbeM", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_MUXED_ENCODER_SEL,  {"Muxed Encoder Select 0", "Muxed Encoder select 1", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_MUXED_ENCODER_MIM,  {"MuxQ-A", "MuxQ-B", "MuxQ-IDX", "MuxQ-IDXM", "Quad-ProbeM", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_MUXED_ENCODER_SEL_MIM,  {"MuxSel0", "MuxSel1", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_STEPGEN,  {"Step/Table1", "Dir/Table2", "Table3", "Table4", "Table5", "Table6", "SGindex", "SGProbe", "Null9", "Null10"}},
  {HM2_GTAG_PWMGEN,      {"PWM/Up", "Dir/Down", "/Enable", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_TPPWM,    {"PWMA", "PWMB", "PWMC", "NPWMA", "NPWMB", "NPWMC", "/ENA", "FAULT", "Null9", "Null10"}},
  {HM2_GTAG_WAVEGEN,  {"PDMA", "PDMB", "Trig0", "Trig1", "Trig2", "Trig3", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_DAQ_FIFO,  {"Data", "Strobe", "Full", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_BIN_OSC,  {"OscOut", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_BIN_DMDMA,  {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_RESOLVER,  {"PwrEn", "PDMP", "PDMM", "ADChan0", "ADChan1", "ADChan2", "SPICS", "SPIClk", "SPIDI0", "SPIDI1"}},
  {HM2_GTAG_SSERIAL,  {"RXData", "TXData", "TXEn", "TestPin", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_TWIDDLER,  {"InBit", "IOBit", "OutBit", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_SPI,      {"/Frame", "DOut", "SClk", "DIn", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_BSPI,     {"/Frame", "DOut", "SClk", "DIn", "CS0", "CS1", "CS2", "CS3", "Null9", "Null10"}},
  {HM2_GTAG_DBSPI,    {"Null1", "DOut", "SClk", "DIn", "/CS-FRM0", "/CS-FRM1", "/CS-FRM2", "/CS-FRM3", "Null9", "Null10"}},
  {HM2_GTAG_DPLL,     {"Sync", "DDSMSB", "FOut", "PostOut", "SyncToggle", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_SSI,      {"SClk", "SClkEn", "Din", "DAv", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_UART_TX,   {"TXData", "TXEna", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_UART_RX,   {"RXData", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_TRAM,    {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
  {HM2_GTAG_LED,      {"Null1", "Null2", "Null3", "Null4", "Null5", "Null6", "Null7", "Null8", "Null9", "Null10"}},
};

static mod_name_t mod_names[HM2_MAX_TAGS] = {
    {"Null",        HM2_GTAG_NONE},
    {"IRQLogic",    HM2_GTAG_IRQ_LOGIC},
    {"WatchDog",    HM2_GTAG_WATCHDOG},
    {"IOPort",      HM2_GTAG_IOPORT},
    {"QCount",      HM2_GTAG_ENCODER},
    {"StepGen",     HM2_GTAG_STEPGEN},
    {"PWM",         HM2_GTAG_PWMGEN},
    {"SPI",         HM2_GTAG_SPI},
    {"SSI",         HM2_GTAG_SSI},
    {"UARTTX",      HM2_GTAG_UART_TX},
    {"UARTRX",      HM2_GTAG_UART_RX},
    {"AddrX",       HM2_GTAG_TRAM},
    {"MuxedQCount", HM2_GTAG_MUXED_ENCODER},
    {"MuxedQCountSel", HM2_GTAG_MUXED_ENCODER_SEL},
    {"BufSPI",      HM2_GTAG_BSPI},
    {"DBufSPI",     HM2_GTAG_DBSPI},
    {"DPLL",        HM2_GTAG_DPLL},
    {"MuxQCntM",    HM2_GTAG_MUXED_ENCODER_MIM},
    {"MuxQSelM",    HM2_GTAG_MUXED_ENCODER_SEL_MIM},
    {"TPPWM",       HM2_GTAG_TPPWM},
    {"WaveGen",     HM2_GTAG_WAVEGEN},
    {"DAQFIFO",     HM2_GTAG_DAQ_FIFO},
    {"BinOsc",      HM2_GTAG_BIN_OSC},
    {"DMDMA",       HM2_GTAG_BIN_DMDMA},
    {"DPLL",        HM2_GTAG_HM2DPLL},
    {"LED",         HM2_GTAG_LED},
    {"ResolverMod", HM2_GTAG_RESOLVER},
    {"SSerial",     HM2_GTAG_SSERIAL},
    {"Twiddler",    HM2_GTAG_TWIDDLER},
};

static mod_name_t mod_names_xml[HM2_MAX_TAGS] = {
    {"Null",        HM2_GTAG_NONE},
    {"IRQLogic",    HM2_GTAG_IRQ_LOGIC},
    {"Watchdog",    HM2_GTAG_WATCHDOG},
    {"IOPort",      HM2_GTAG_IOPORT},
    {"Encoder",     HM2_GTAG_ENCODER},
    {"StepGen",     HM2_GTAG_STEPGEN},
    {"PWM",         HM2_GTAG_PWMGEN},
    {"SPI",         HM2_GTAG_SPI},
    {"SSI",         HM2_GTAG_SSI},
    {"UARTTX",      HM2_GTAG_UART_TX},
    {"UARTRX",      HM2_GTAG_UART_RX},
    {"AddrX",       HM2_GTAG_TRAM},
    {"MuxedQCount", HM2_GTAG_MUXED_ENCODER},
    {"MuxedQCountSel", HM2_GTAG_MUXED_ENCODER_SEL},
    {"BufSPI",      HM2_GTAG_BSPI},
    {"DBufSPI",     HM2_GTAG_DBSPI},
    {"DPLL",        HM2_GTAG_DPLL},
    {"MuxQCntM",    HM2_GTAG_MUXED_ENCODER_MIM},
    {"MuxQSelM",    HM2_GTAG_MUXED_ENCODER_SEL_MIM},
    {"TPPWM",       HM2_GTAG_TPPWM},
    {"WaveGen",     HM2_GTAG_WAVEGEN},
    {"DAQFIFO",     HM2_GTAG_DAQ_FIFO},
    {"BinOsc",      HM2_GTAG_BIN_OSC},
    {"DMDMA",       HM2_GTAG_BIN_DMDMA},
    {"DPLL",        HM2_GTAG_HM2DPLL},
    {"LED",         HM2_GTAG_LED},
    {"ResolverMod", HM2_GTAG_RESOLVER},
    {"SSerial",     HM2_GTAG_SSERIAL},
    {"Twiddler",    HM2_GTAG_TWIDDLER},
};

static char *pin_find_module_name(int gtag, int xml_flag) {
    int i;
    mod_name_t *mod_names_ptr;

    if (gtag == HM2_GTAG_NONE) {
        return "None";
    }

    if (xml_flag == 0) {
        mod_names_ptr = mod_names;
    } else {
        mod_names_ptr = mod_names_xml;
    }

    for (i = 0; i < HM2_MAX_TAGS; i++) {
        if (mod_names_ptr[i].tag == gtag)
            return mod_names_ptr[i].name;
    }
    return "(unknown-gtag)";
}


static char *pin_get_pin_name(hm2_pin_desc_t *pin, int xml_flag) {
    int i;
    u8 chan;
    static char buff[100];
    pin_name_t *pin_names_ptr;
    
    if (xml_flag == 0) {
        pin_names_ptr = pin_names;
    } else {
        pin_names_ptr = pin_names_xml;
    }

    for (i = 0; i < HM2_MAX_TAGS; i++) {
        if (pin_names_ptr[i].tag == pin->sec_tag) {
            if (pin->sec_tag == HM2_GTAG_SSERIAL) {
                chan = pin->sec_pin & 0x0F;
                if ((pin->sec_pin & 0xF0) == 0x00) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[0], chan);
                    return buff;
                } else if ((pin->sec_pin & 0xF0) == 0x80) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[1], chan);
                    return buff;
                } else if ((pin->sec_pin & 0xF0) == 0x90) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[2], chan);
                    return buff;
                } else if (pin->sec_pin == 0xA1) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[3], chan);
                    return buff;
                }
            } else if (pin->gtag == HM2_GTAG_DAQ_FIFO) {
                chan = pin->sec_chan & 0x1F;
                if ((pin->sec_pin & 0xE0) == 0x00) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[0], chan);
                    return buff;
                } else if (pin->sec_pin == 0x41) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[1], chan);
                    return buff;
                } else if (pin->sec_pin == 0x81) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[2], chan);
                    return buff;
                }
            } else if (pin->gtag == HM2_GTAG_TWIDDLER) {
                chan = pin->sec_chan & 0x1F;
                if ((pin->sec_pin & 0xC0) == 0x00) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[0], chan);
                    return buff;
                } else if ((pin->sec_pin & 0xC0) == 0xC0) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[1], chan);
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[1], chan);
                    return buff;
                } else if ((pin->sec_pin & 0xC0) == 0x80) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[2], chan);
                    return buff;
                }
            } else if (pin->gtag == HM2_GTAG_BIN_OSC) {
                chan = pin->sec_chan & 0x1F;
                if ((pin->sec_pin & 0x80) == 0x80) {
                    sprintf(buff, "%s%u", pin_names_ptr[i].name[0], chan);
                    return buff;
                }
            } else {
                sprintf(buff, "%s", pin_names_ptr[i].name[(pin->sec_pin & 0x0F) - 1]);
                return buff;
            }
        };
    };
    sprintf(buff, "Unknown");
    return buff;
}

void hm2_print_pin_file(llio_t *llio, int xml_flag) {
    unsigned int i, j;
    const u8 db25_pins[] = {1, 14, 2, 15, 3, 16, 4, 17, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    if (xml_flag == 0) {
        printf("Configuration Name: %.*s\n", 8, llio->hm2.config_name);
        printf("\nGeneral configuration information:\n\n");
        printf("  BoardName : %.*s\n", (int)sizeof(llio->hm2.idrom.board_name), llio->hm2.idrom.board_name);
        printf("  FPGA Size: %u KGates\n", llio->hm2.idrom.fpga_size);
        printf("  FPGA Pins: %u\n", llio->hm2.idrom.fpga_pins);
        printf("  Number of IO Ports: %u\n", llio->hm2.idrom.io_ports);
        printf("  Width of one I/O port: %u\n", llio->hm2.idrom.port_width);
        printf("  Clock Low frequency: %.4f MHz\n", (float) llio->hm2.idrom.clock_low/1000000.0);
        printf("  Clock High frequency: %.4f MHz\n", (float) llio->hm2.idrom.clock_high/1000000.0);
        printf("  IDROM Type: %u\n", llio->hm2.idrom.idrom_type);
        printf("  Instance Stride 0: %u\n", llio->hm2.idrom.instance_stride0);
        printf("  Instance Stride 1: %u\n", llio->hm2.idrom.instance_stride1);
        printf("  Register Stride 0: %u\n", llio->hm2.idrom.register_stride0);
        printf("  Register Stride 1: %u\n", llio->hm2.idrom.register_stride1);

        printf("\nModules in configuration:\n\n");
        for (i = 0; i < HM2_MAX_MODULES; i++) {
            if ((llio->hm2.modules[i].gtag == 0) && (llio->hm2.modules[i].version == 0) &&
            (llio->hm2.modules[i].clock_tag == 0) && (llio->hm2.modules[i].instances == 0)) break;

            {
                int k;
                for (k = 0; k < HM2_MAX_TAGS; k++) {
                    if (mod_names[k].tag == llio->hm2.modules[i].gtag) {
                        printf("  Module: %s\n", mod_names[k].name);
                        printf("  There are %u of %s in configuration\n", llio->hm2.modules[i].instances, mod_names[k].name);
                        break;
                    }
                }
            }

            printf("  Version: %u\n", llio->hm2.modules[i].version);
            printf("  Registers: %u\n", llio->hm2.modules[i].registers);
            printf("  BaseAddress: %04X\n", llio->hm2.modules[i].base_address);
            printf("  ClockFrequency: %.3f MHz\n",
              llio->hm2.modules[i].clock_tag == HM2_CLOCK_LOW_TAG ? (float) llio->hm2.idrom.clock_low/1000000.0 : (float) llio->hm2.idrom.clock_high/1000000.0);
            printf("  Register Stride: %u bytes\n",
              (llio->hm2.modules[i].strides & 0x0F) == 0 ? llio->hm2.idrom.register_stride0 : llio->hm2.idrom.register_stride1);
            printf("  Instance Stride: %u bytes\n\n",
              (llio->hm2.modules[i].strides & 0xF0) == 0 ? llio->hm2.idrom.instance_stride0 : llio->hm2.idrom.instance_stride1);
        }

        printf("Configuration pin-out:\n");
        for (i = 0; i < llio->hm2.idrom.io_ports; i++) {
            printf("\nIO Connections for %s\n", llio->ioport_connector_name[i]);
            printf("Pin#  I/O   Pri. func    Sec. func       Chan      Pin func        Pin Dir\n\n");
            for (j = 0; j < llio->hm2.idrom.port_width; j++) {
                hm2_pin_desc_t *pin = &(llio->hm2.pins[i*(llio->hm2.idrom.port_width) + j]);
                int pin_nr;

                switch (llio->hm2.idrom.port_width) {
                    case 17:
                        pin_nr = db25_pins[(i*(llio->hm2.idrom.port_width) + j) % 17];
                        break;
                    case 24:
                        pin_nr = (i*(llio->hm2.idrom.port_width) + j) % (llio->hm2.idrom.port_width)*2 + 1;
                        break;
                    case 32:
                        pin_nr = i*(llio->hm2.idrom.port_width) + j;
                        break;
                }
                printf("%2u", pin_nr);
                printf("    %3u", i*(llio->hm2.idrom.port_width) + j);
                printf("   %-8s", pin_find_module_name(pin->gtag, xml_flag));
                if (pin->sec_tag == HM2_GTAG_NONE) {
                    printf("     %-15s", "None");
                } else {
                    printf("     %-15s", pin_find_module_name(pin->sec_tag, xml_flag));

                    if (pin->sec_chan & HM2_CHAN_GLOBAL) {
                        printf("    Global    ");
                    } else {
                        printf(" %2u        ", pin->sec_chan);
                    }
                    printf("%-12s", pin_get_pin_name(pin, xml_flag));

                    if (pin->sec_pin & HM2_PIN_OUTPUT) {
                        printf("    (Out)");
                    } else {
                        printf("    (In)");
                    }
                }

                printf("\n");
            }
        }
        printf("\n");
    } else {
        printf("<?xml version=\"1.0\"?>\n");
        printf("<hostmot2>\n");
        printf("    <boardname>%.*s</boardname>\n", (int)sizeof(llio->hm2.idrom.board_name), llio->hm2.idrom.board_name);
        printf("    <ioports>%d</ioports>\n", llio->hm2.idrom.io_ports);
        printf("    <iowidth>%d</iowidth>\n", llio->hm2.idrom.port_width*llio->hm2.idrom.io_ports);
        printf("    <portwidth>%d</portwidth>\n", llio->hm2.idrom.port_width);
        printf("    <clocklow>%8d</clocklow>\n", llio->hm2.idrom.clock_low);
        printf("    <clockhigh>%8d</clockhigh>\n", llio->hm2.idrom.clock_high);

        printf("    <modules>\n");
        for (i = 0; i < HM2_MAX_MODULES; i++) {
            if ((llio->hm2.modules[i].gtag == 0) && (llio->hm2.modules[i].version == 0) &&
            (llio->hm2.modules[i].clock_tag == 0) && (llio->hm2.modules[i].instances == 0)) break;

            printf("        <module>\n");
            {
                int k;
                for (k = 0; k < HM2_MAX_TAGS; k++) {
                    if (mod_names[k].tag == llio->hm2.modules[i].gtag) {
                        printf("            <tagname>%s</tagname>\n", mod_names[k].name);
                        printf("            <numinstances>%2d</numinstances>\n", llio->hm2.modules[i].instances);
                        break;
                    }
                }
            }

            printf("        </module>\n");
        }
        printf("    </modules>\n");

        printf("    <pins>\n");
        for (i = 0; i < llio->hm2.idrom.io_ports; i++) {
            for (j = 0; j < llio->hm2.idrom.port_width; j++) {
                hm2_pin_desc_t *pin = &(llio->hm2.pins[i*(llio->hm2.idrom.port_width) + j]);
                int pin_nr;

                switch (llio->hm2.idrom.port_width) {
                    case 17:
                        pin_nr = db25_pins[(i*(llio->hm2.idrom.port_width) + j) % 17];
                        break;
                    case 24:
                        pin_nr = (i*(llio->hm2.idrom.port_width) + j) % (llio->hm2.idrom.port_width)*2 + 1;
                        break;
                    case 32:
                        pin_nr = i*(llio->hm2.idrom.port_width) + j;
                        break;
                }
                printf("        <pin>\n");
                printf("            <connector>%s</connector>\n", llio->ioport_connector_name[i]);
                printf("            <secondarymodulename>%s</secondarymodulename>\n", pin_find_module_name(pin->sec_tag, xml_flag));
                printf("            <secondaryfunctionname>");
                if (pin->sec_tag != HM2_GTAG_NONE) {
                    printf("%s", pin_get_pin_name(pin, xml_flag));
                    if (pin->sec_pin & HM2_PIN_OUTPUT) {
                        printf(" (Out)");
                    } else {
                        printf(" (In)");
                    }
                }
                printf("</secondaryfunctionname>\n");
                printf("            <secondaryinstance>%2d</secondaryinstance>\n", pin_nr);
                printf("        </pin>\n");
            }
        }
        printf("    </pins>\n");
        printf("</hostmot2>\n");
    }
}
