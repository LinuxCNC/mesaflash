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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "hostmot2.h"
#include "sserial_module.h"

// Temporarily enable the pins that are not masked by sserial_mode
static void enable_sserial_pins(llio_t *llio) {
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
static void disable_sserial_pins(llio_t *llio) {
    int port_pin, port;
    u32 ddr_reg = 0;
    u32 src_reg = 0;

    for (port = 0; port < 2; port ++) {
        llio->write(llio, 0x1100 + 4*port, &ddr_reg, sizeof(u32));
        llio->write(llio, 0x1200 + 4*port, &src_reg, sizeof(u32));
    }
}

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
    char *ptr = (char *) buffer;

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

int sserial_init(sserial_module_t *ssmod, board_t *board, int interface_num, int channel_num, u32 remote_type) {
    u32 cmd, status, data, addr;
    u16 d;
    int i;
    hm2_module_desc_t *md = hm2_find_module(&(board->llio.hm2), HM2_GTAG_SSERIAL);

    if (md == NULL) {
        printf("No sserial module found.\n");
        return -1;
    }
    if (interface_num >= HM2_SSERIAL_MAX_INTEFACES) {
        printf("sserial inteface number too high.\n");
        return -1;
    }
    if (channel_num >= HM2_SSERIAL_MAX_CHANNELS) {
        printf("sserial channel number too high.\n");
        return -1;
    }

    memset(ssmod, 0, sizeof(sserial_module_t));
    ssmod->board = board;
    ssmod->interface_num = interface_num;
    ssmod->channel_num = channel_num;
    ssmod->instance_stride = (md->strides & 0xF0) == 0 ? board->llio.hm2.idrom.instance_stride0 : board->llio.hm2.idrom.instance_stride1;

    enable_sserial_pins(&(ssmod->board->llio));
    sslbp_send_local_cmd(&(ssmod->board->llio), 0, SSLBP_CMD_RESET);

    ssmod->interface.type = sslbp_read_local8(&(ssmod->board->llio), interface_num, SSLBP_TYPE_LOC);
    ssmod->interface.width = sslbp_read_local8(&(ssmod->board->llio), interface_num, SSLBP_WIDTH_LOC);
    ssmod->interface.ver_major = sslbp_read_local8(&(ssmod->board->llio), interface_num, SSLBP_MAJOR_REV_LOC);
    ssmod->interface.ver_minor = sslbp_read_local8(&(ssmod->board->llio), interface_num, SSLBP_MINOR_REV_LOC);
    ssmod->interface.gp_inputs = sslbp_read_local8(&(ssmod->board->llio), interface_num, SSLBP_CHANNEL_START_LOC);
    ssmod->interface.gp_outputs = sslbp_read_local8(&(ssmod->board->llio), interface_num, SSLBP_CHANNEL_STRIDE_LOC);
    ssmod->interface.processor_type = sslbp_read_local8(&(ssmod->board->llio), interface_num, SSLBP_PROCESSOR_TYPE_LOC);
    ssmod->interface.channels_count = sslbp_read_local8(&(ssmod->board->llio), interface_num, SSLBP_NR_CHANNELS_LOC);
    ssmod->interface.baud_rate = sslbp_read_local32(&(ssmod->board->llio), interface_num, ssmod->interface.gp_inputs + 42);
    ssmod->interface.clock = sslbp_read_local32(&(ssmod->board->llio), interface_num, SSLBP_CLOCK_LOC);

    if (0) {//ssmod->board->llio.verbose == 1) {
        printf("SSLBP port %d:\n", interface_num);
        printf("  interface type: %0x\n", ssmod->interface.type);
        printf("  interface width: %d\n", ssmod->interface.width);
        printf("  SSLBP Version: %d.%d\n", ssmod->interface.ver_major, ssmod->interface.ver_minor);
        printf("  SSLBP Channel Start: %d\n", ssmod->interface.gp_inputs);
        printf("  SSLBP Channel Stride: %d\n", ssmod->interface.gp_outputs);
        printf("  SSLBP Processor Type: %x\n", ssmod->interface.processor_type);
        printf("  SSLBP Channels: %d\n", ssmod->interface.channels_count);
        printf("  SSLBP Baud Rate: %.1f Mb\n", ssmod->interface.baud_rate/1000000.0);
        printf("  SSLBP Clock: %u MHz\n", ssmod->interface.clock/1000000);
    }

    cmd = 0;
    board->llio.write(&(ssmod->board->llio), HM2_MODULE_SSERIAL_CS + interface_num*ssmod->instance_stride + channel_num*4, &(cmd), sizeof(u32));

    sslbp_send_local_cmd(&(ssmod->board->llio), interface_num, SSLBP_CMD_START_SETUP_MODE(channel_num));
    sslbp_wait_complete(&(ssmod->board->llio), interface_num);
    if (sslbp_read_data(&(ssmod->board->llio), interface_num) != 0) {
        printf("Error reading sserial interface %d channel %d\n", interface_num, channel_num);
        return -1;
    }

    board->llio.read(&(ssmod->board->llio), HM2_MODULE_SSERIAL_CS + interface_num*ssmod->instance_stride + channel_num*4, &(status), sizeof(u32));
    board->llio.read(&(ssmod->board->llio), HM2_MODULE_SSERIAL_INTERFACE0 + interface_num*ssmod->instance_stride + channel_num*4, &(status), sizeof(u32));
    if ((status & 0xFF000000) != remote_type) {
        printf("Found wrong remote at %d:%d, reqeust %x but found %x\n", interface_num, channel_num, remote_type, status);
        return -1;
    }
    ssmod->device.unit = status;
    board->llio.read(&(ssmod->board->llio), HM2_MODULE_SSERIAL_INTERFACE1 + interface_num*ssmod->instance_stride + channel_num*4, &(ssmod->device.name), sizeof(u32));
    board->llio.read(&(ssmod->board->llio), HM2_MODULE_SSERIAL_INTERFACE2 + interface_num*ssmod->instance_stride + channel_num*4, &(status), sizeof(u32));

    printf("device at %d:%d: %.*s (unit 0x%08X)\n", interface_num, channel_num, 4, ssmod->device.name, ssmod->device.unit);

    sslbp_send_local_cmd(&(ssmod->board->llio), 0, SSLBP_CMD_STOPALL);
    sslbp_wait_complete(&(ssmod->board->llio), 0);
    cmd = 0;
    board->llio.write(&(ssmod->board->llio), HM2_MODULE_SSERIAL_CS + channel_num*4, &(cmd), sizeof(cmd));
    printf("starting device %d:%d\n", interface_num, channel_num);
    sslbp_send_local_cmd(&(ssmod->board->llio), 0, SSLBP_CMD_START_NORMAL_MODE(channel_num));
    sslbp_wait_complete(&(ssmod->board->llio), 0);
    board->llio.read(&(ssmod->board->llio), HM2_MODULE_SSERIAL_DATA + channel_num*4, &(status), sizeof(u32));
    if (status != 0) {
        printf("Error while starting sserial: %X\n", status);
        return -1;
    }
    return 0;
}

int sserial_cleanup(sserial_module_t *ssmod) {
    disable_sserial_pins(&(ssmod->board->llio));

    return 0;
}

int sserial_write(sserial_module_t *ssmod) {
    ssmod->board->llio.write(&(ssmod->board->llio), HM2_MODULE_SSERIAL_INTERFACE0 + ssmod->interface_num*ssmod->instance_stride + ssmod->channel_num*4, &(ssmod->interface0), sizeof(u32));
    ssmod->board->llio.write(&(ssmod->board->llio), HM2_MODULE_SSERIAL_INTERFACE1 + ssmod->interface_num*ssmod->instance_stride + ssmod->channel_num*4, &(ssmod->interface1), sizeof(u32));
    ssmod->board->llio.write(&(ssmod->board->llio), HM2_MODULE_SSERIAL_INTERFACE2 + ssmod->interface_num*ssmod->instance_stride + ssmod->channel_num*4, &(ssmod->interface2), sizeof(u32));
    sslbp_send_local_cmd(&(ssmod->board->llio), 0, SSLBP_CMD_DOIT(1));
    sslbp_wait_complete(&(ssmod->board->llio), 0);
    ssmod->board->llio.read(&(ssmod->board->llio), HM2_MODULE_SSERIAL_DATA + ssmod->interface_num*ssmod->instance_stride + ssmod->channel_num*4, &(ssmod->data), sizeof(u32));
    ssmod->board->llio.read(&(ssmod->board->llio), HM2_MODULE_SSERIAL_CS + ssmod->interface_num*ssmod->instance_stride + ssmod->channel_num*4, &(ssmod->cs), sizeof(u32));

    return 0;
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
        llio->ss_interface[i].ver_major = sslbp_read_local8(llio, i, SSLBP_MAJOR_REV_LOC);
        llio->ss_interface[i].ver_minor = sslbp_read_local8(llio, i, SSLBP_MINOR_REV_LOC);
        llio->ss_interface[i].gp_inputs = sslbp_read_local8(llio, i, SSLBP_CHANNEL_START_LOC);
        llio->ss_interface[i].gp_outputs = sslbp_read_local8(llio, i, SSLBP_CHANNEL_STRIDE_LOC);
        llio->ss_interface[i].processor_type = sslbp_read_local8(llio, i, SSLBP_PROCESSOR_TYPE_LOC);
        llio->ss_interface[i].channels_count = sslbp_read_local8(llio, i, SSLBP_NR_CHANNELS_LOC);
        llio->ss_interface[i].baud_rate = sslbp_read_local32(llio, i, llio->ss_interface[i].gp_inputs + 42);
        llio->ss_interface[i].clock = sslbp_read_local32(llio, i, SSLBP_CLOCK_LOC);

        printf("SSLBP port %d:\n", i);
        printf("  interface type: %0x\n", llio->ss_interface[i].type);
        printf("  interface width: %d\n", llio->ss_interface[i].width);
        printf("  SSLBP Version: %d.%d\n", llio->ss_interface[i].ver_major, llio->ss_interface[i].ver_minor);
        printf("  SSLBP Channel Start: %d\n", llio->ss_interface[i].gp_inputs);
        printf("  SSLBP Channel Stride: %d\n", llio->ss_interface[i].gp_outputs);
        printf("  SSLBP Processor Type: %x\n", llio->ss_interface[i].processor_type);
        printf("  SSLBP Channels: %d\n", llio->ss_interface[i].channels_count);
        printf("  SSLBP Baud Rate: %d\n", llio->ss_interface[i].baud_rate);
        printf("  SSLBP Clock: %u MHz\n", llio->ss_interface[i].clock/1000000);

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
                record_type = sslbp_read_remote8(llio, i, channel, d);
                addr += 2;
                if (llio->verbose == 0)
                    continue;
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
            }

            addr = (status >> 16) & 0xFFFF;
            while (1) {
                sserial_pdd_t sserial_pdd;
                sserial_md_t sserial_md;
                u8 record_type;
                char name[48], unit[48], buff[32];

                d = sslbp_read_remote16(llio, i, channel, addr);
                if (d == 0) break;
                record_type = sslbp_read_remote8(llio, i, channel, d);
                addr += 2;
                if (record_type == LBP_DATA) {
                    sslbp_read_remote_bytes(llio, i, channel, d, &(sserial_pdd), sizeof(sserial_pdd_t));
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_pdd_t), &(unit), -1);
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_pdd_t) + strlen(unit) + 1, &(name), -1);
                    if (llio->verbose == 1) {
                        printf("    RecordType: %02X\n", sserial_pdd.record_type);
                        printf("    DataSize: %02X\n", sserial_pdd.data_size);
                        printf("    DataType: %02X\n", sserial_pdd.data_type);
                        printf("    DataDir: %02X\n", sserial_pdd.data_dir);
                        printf("    param min: %f\n", sserial_pdd.param_min);
                        printf("    param max: %f\n", sserial_pdd.param_max);
                        printf("    param addr: %04X\n", sserial_pdd.param_addr);
                        printf("    unit: %s\n", unit);
                        printf("    name: %s\n", name);
                    }
                    if (strncmp(name, "SwRevision", 10) == 0) {
                        sslbp_read_remote_bytes(llio, i, channel, sserial_pdd.param_addr, &(d), sserial_pdd.data_size/8);
                        llio->ss_device[channel].sw_revision = d;
                    }
                } else if (record_type == LBP_MODE) {
                    sslbp_read_remote_bytes(llio, i, channel, d, &(sserial_md), sizeof(sserial_md_t));
                    sslbp_read_remote_bytes(llio, i, channel, d + sizeof(sserial_md_t), &(name), -1);
                    if (llio->verbose == 1) {
                        printf("    record type: %02X\n", sserial_md.record_type);
                        printf("    mode index: %02X\n", sserial_md.mode_index);
                        printf("    mode type: %02X\n", sserial_md.mode_type);
                        printf("    unused: %02X\n", sserial_md.unused);
                        printf("    mode name: %s\n", name);
                    }
                }
            }

            printf("  sserial device at channel %d: %.*s", channel, 4, llio->ss_device[channel].name);
            if ((llio->ss_device[channel].unit & 0xFF000000) == SSLBP_REMOTE_7I77_IO) {
                printf(" IO");
            }
            printf(" (unit 0x%08X, sw revision: %u)\n", llio->ss_device[channel].unit, llio->ss_device[channel].sw_revision);
        }
    }
    disable_sserial_pins(llio);
}
