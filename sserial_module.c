
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "anyio.h"
#include "hostmot2.h"

void sslbp_send_local_cmd(llio_t *llio, int interface, u32 cmd) {
    llio->write(llio, HM2_MODULE_SSERIAL_CMD + interface*0x40, &(cmd), sizeof(u32));
}

u32 sslbp_read_local_cmd(llio_t *llio, int interface) {
    u32 data;

    llio->read(llio, HM2_MODULE_SSERIAL_CMD + interface*0x40, &(data), sizeof(u32));
    return data;
}

u8 sslbp_read_data(llio_t *llio, int interface) {
    u32 data;

    llio->read(llio, HM2_MODULE_SSERIAL_DATA + interface*0x40, &(data), sizeof(u32));
    return data & 0xFF;
}

void sslbp_wait_complete(llio_t *llio, int interface) {
    while (sslbp_read_local_cmd(llio, interface) != 0) {}
}

void sslbp_send_remote_cmd(llio_t *llio, int interface, int channel, u32 cmd) {
    llio->write(llio, HM2_MODULE_SSERIAL_CS + interface*0x40 + channel*4, &(cmd), sizeof(u32));
}

u8 sslbp_read_local8(llio_t *llio, int interface, u32 addr) {
    u8 ret;

    sslbp_send_local_cmd(llio, interface, SSLBP_CMD_READ(addr));
    sslbp_wait_complete(llio, interface);
    return sslbp_read_data(llio, interface);
}

u32 sslbp_read_local32(llio_t *llio, int interface, u32 addr) {
    int byte = 4;
    u32 ret = 0;

    for (; byte--;)
        ret = (ret << 8) | sslbp_read_local8(llio, interface, addr + byte);
    return ret;
}

u8 sslbp_read_remote8(llio_t *llio, int interface, int channel, u32 addr) {
    u32 data;

    sslbp_send_remote_cmd(llio, interface, channel, 0x4C000000 | addr);
    sslbp_send_local_cmd(llio, interface, SSLBP_CMD_DOIT(channel));
    sslbp_wait_complete(llio, interface);
    llio->read(llio, HM2_MODULE_SSERIAL_INTERFACE0 + interface*0x40 + channel*4, &(data), sizeof(u32));
    return data & 0xFF;
}

u16 sslbp_read_remote16(llio_t *llio, int interface, int channel, u32 addr) {
    int byte;
    u16 ret = 0;

    for (byte = 1; byte >= 0; byte--)
        ret = (ret << 8) | sslbp_read_remote8(llio, interface, channel, addr + byte);
    return ret;
}

u32 sslbp_read_remote32(llio_t *llio, int interface, int channel, u32 addr) {
    int byte;
    u32 ret = 0;

    for (byte = 3; byte >=0; byte--)
        ret = (ret << 8) | sslbp_read_remote8(llio, interface, channel, addr + byte);
    return ret;
}

void sslbp_read_remote_bytes(llio_t *llio, int interface, int channel, u32 addr, void *buffer, int size) {
    char *ptr = (u8 *) buffer;

    while (size != 0) {
        u8 data = sslbp_read_remote8(llio, interface, channel, addr);
        *(ptr++) = data;
        addr++;
        size--;
        if (size < 0) {
            if (data == 0)
                break;
        }
    }
}

// Temporarily enable the pins that are not masked by sserial_mode
void enable_sserial_pins(llio_t *llio) {
    int port_pin, port;
    int pin = -1;
    int chan_counts[] = {0,0,0,0,0,0,0,0};

    for (port  = 0; port < 2; port ++) {
        u32 ddr_reg = 0;
        u32 src_reg = 0;
        for (port_pin = 0; port_pin < llio->hm2.idrom.port_width; port_pin++) {
            pin++;
            if (llio->hm2.pins[pin].sec_tag == HM2_GTAG_SSERIAL) {
                // look for highest-indexed pin to determine number of channels
                if ((llio->hm2.pins[pin].sec_pin & 0x0F) > chan_counts[llio->hm2.pins[pin].sec_chan]) {
                    chan_counts[llio->hm2.pins[pin].sec_chan] = (llio->hm2.pins[pin].sec_pin & 0x0F);
                }
                // check if the channel is enabled
                //printf("sec unit = %i, sec pin = %i\n", llio->hm2.pins[pin].sec_chan, llio->hm2.pins[pin].sec_pin & 0x0F);
                    src_reg |= (1 << port_pin);
                    if (llio->hm2.pins[pin].sec_pin & 0x80) 
                        ddr_reg |= (1 << port_pin);
            }
        }
        llio->write(llio, 0x1100 + 4*port, &ddr_reg, sizeof(u32));
        llio->write(llio, 0x1200 + 4*port, &src_reg, sizeof(u32));
    }
}

// Return the physical ports to default
void disable_sserial_pins(llio_t *llio) {
    int port_pin, port;
    u32 ddr_reg = 0;
    u32 src_reg = 0;

    for (port = 0; port < 2; port ++) {
        llio->write(llio, 0x1100 + 4*port, &ddr_reg, sizeof(u32));
        llio->write(llio, 0x1200 + 4*port, &src_reg, sizeof(u32));
    }
}

void sserial_module_init(llio_t *llio) {
    u32 cmd, status, data, addr;
    u16 d;
    int i, channel;
    hm2_module_desc_t *sserial_mod = hm2_find_module(&(llio->hm2), HM2_GTAG_SSERIAL);

    if (sserial_mod == NULL)
        return;

    enable_sserial_pins(llio);
    sslbp_send_local_cmd(llio, 0, SSLBP_CMD_RESET);

    for (i = 0; i < 1; i++) {

        llio->ss_interface[i].type = sslbp_read_local8(llio, i, 0);
        llio->ss_interface[i].width = sslbp_read_local8(llio, i, 1);
        llio->ss_interface[i].ver_major = sslbp_read_local8(llio, i, 2);
        llio->ss_interface[i].ver_minor = sslbp_read_local8(llio, i, 3);
        llio->ss_interface[i].gp_inputs = sslbp_read_local8(llio, i, 4);
        llio->ss_interface[i].gp_outputs = sslbp_read_local8(llio, i, 5);
        llio->ss_interface[i].processor_type = sslbp_read_local8(llio, i, 6);
        llio->ss_interface[i].channels_count = sslbp_read_local8(llio, i, 7);
        llio->ss_interface[i].baud_rate = sslbp_read_local32(llio, i, llio->ss_interface[i].gp_inputs + 42);

        printf("SSERIAL INTERFACE %d:\n", i);
        if (llio->verbose == 1) {
            printf("  interface type: %0x\n", llio->ss_interface[i].type);
            printf("  interface width: %d\n", llio->ss_interface[i].width);
            printf("  interface version: %d.%d\n", llio->ss_interface[i].ver_major, llio->ss_interface[i].ver_minor);
            printf("  gp inputs: %d\n", llio->ss_interface[i].gp_inputs);
            printf("  gp outputs: %d\n", llio->ss_interface[i].gp_outputs);
            printf("  processor type: %x\n", llio->ss_interface[i].processor_type);
            printf("  channels: %d\n", llio->ss_interface[i].channels_count);
            printf("  baud rate: %d baud\n", llio->ss_interface[i].baud_rate);
        }

        for (channel = 0; channel < llio->ss_interface[i].width; channel++) {
            cmd = 0;
            llio->write(llio, HM2_MODULE_SSERIAL_CS + i*0x40 + channel*4, &(cmd), sizeof(u32));

            sslbp_send_local_cmd(llio, i, SSLBP_CMD_START_SETUP_MODE(channel));
            sslbp_wait_complete(llio, i);
            if (sslbp_read_data(llio, i) != 0)
                continue;

            llio->read(llio, HM2_MODULE_SSERIAL_CS + i*0x40 + channel*4, &(status), sizeof(u32));
            llio->read(llio, HM2_MODULE_SSERIAL_INTERFACE0 + i*0x40 + channel*4, &(status), sizeof(u32));
            llio->ss_device[channel].unit = status;
            llio->read(llio, HM2_MODULE_SSERIAL_INTERFACE1 + i*0x40 + channel*4, &(status), sizeof(u32));
            llio->ss_device[channel].name[0] = status & 0xFF;
            llio->ss_device[channel].name[1] = (status >> 8) & 0xFF;
            llio->ss_device[channel].name[2] = (status >> 16) & 0xFF;
            llio->ss_device[channel].name[3] = (status >> 24) & 0xFF;
            llio->read(llio, HM2_MODULE_SSERIAL_INTERFACE2 + i*0x40 + channel*4, &(status), sizeof(u32));

            addr = status & 0xFFFF;
            while (1) {
                sserial_pdd_t sserial_pdd;
                sserial_md_t sserial_md;
                u8 record_type;
                char name[48];

                d = sslbp_read_remote16(llio, i, channel, addr);
                if (d == 0) break;
                printf("PTOCP %04X\n", d);
                record_type = sslbp_read_remote8(llio, i, channel, d);
                if (record_type == LBP_DATA) {
                    sslbp_read_remote_bytes(llio, i, channel, d, &(sserial_pdd), sizeof(sserial_pdd_t));
                    printf("    RecordType: %02X\n", sserial_pdd.record_type);
                    printf("    DataSize: %02X\n", sserial_pdd.data_size);
                    printf("    DataType: %02X\n", sserial_pdd.data_type);
                    printf("    DataDir: %02X\n", sserial_pdd.data_dir);
                    printf("    param min: %f\n", sserial_pdd.param_min);
                    printf("    param max: %f\n", sserial_pdd.param_max);
                    printf("    param addr: %04X\n", sserial_pdd.param_addr);
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_pdd_t), &(name), -1);
                    printf("    unit: %s\n", name);
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_pdd_t) + strlen(name) + 1, &(name), -1);
                    printf("    name: %s\n", name);
                } else if (record_type == LBP_MODE) {
                    sslbp_read_remote_bytes(llio, i, channel, d, &(sserial_md), sizeof(sserial_md_t));
                    printf("    record type: %02X\n", sserial_md.record_type);
                    printf("    mode index: %02X\n", sserial_md.mode_index);
                    printf("    mode type: %02X\n", sserial_md.mode_type);
                    printf("    unused: %02X\n", sserial_md.unused);
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_md_t), &(name), -1);
                    printf("    mode name: %s\n", name);
                }
                addr += 2;
            }

            addr = (status >> 16) & 0xFFFF;
            while (1) {
                sserial_pdd_t sserial_pdd;
                sserial_md_t sserial_md;
                u8 record_type;
                char name[48];

                d = sslbp_read_remote16(llio, i, channel, addr);
                if (d == 0) break;
                printf("GTOCP %04X\n", d);
                record_type = sslbp_read_remote8(llio, i, channel, d);
                if (record_type == LBP_DATA) {
                    sslbp_read_remote_bytes(llio, i, channel, d, &(sserial_pdd), sizeof(sserial_pdd_t));
                    printf("    RecordType: %02X\n", sserial_pdd.record_type);
                    printf("    DataSize: %02X\n", sserial_pdd.data_size);
                    printf("    DataType: %02X\n", sserial_pdd.data_type);
                    printf("    DataDir: %02X\n", sserial_pdd.data_dir);
                    printf("    param min: %f\n", sserial_pdd.param_min);
                    printf("    param max: %f\n", sserial_pdd.param_max);
                    printf("    param addr: %04X\n", sserial_pdd.param_addr);
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_pdd_t), &(name), -1);
                    printf("    unit: %s\n", name);
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_pdd_t) + strlen(name) + 1, &(name), -1);
                    printf("    name: %s\n", name);
                } else if (record_type == LBP_MODE) {
                    sslbp_read_remote_bytes(llio, i, channel, d, &(sserial_md), sizeof(sserial_md_t));
                    printf("    record type: %02X\n", sserial_md.record_type);
                    printf("    mode index: %02X\n", sserial_md.mode_index);
                    printf("    mode type: %02X\n", sserial_md.mode_type);
                    printf("    unused: %02X\n", sserial_md.unused);
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_md_t), &(name), -1);
                    printf("    mode name: %s\n", name);
                }
                addr += 2;
            }

            printf("  device at channel %d: %.*s (unit 0x%08X)\n", channel, 4, llio->ss_device[channel].name, llio->ss_device[channel].unit);
        }
    }
    disable_sserial_pins(llio);
}
