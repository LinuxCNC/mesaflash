
#include <pci/pci.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "eeprom.h"
#include "lbp16.h"

extern int sd;
extern socklen_t len;
extern struct sockaddr_in server_addr, client_addr;

int lbp16_read(u16 cmd, u32 addr, void *buffer, int size) {
    lbp16_cmd_addr packet;
    int send, recv;
    u8 local_buff[size];

    LBP16_INIT_PACKET4(packet, cmd, addr);
    if (LBP16_SENDRECV_DEBUG)
        printf("SEND: %02X %02X %02X %02X | REQUEST %d bytes\n", packet.cmd_hi, packet.cmd_lo, packet.addr_hi, packet.addr_lo, size);
    send = sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    recv = recvfrom(sd, (char*) &local_buff, sizeof(local_buff), 0, (struct sockaddr *) &client_addr, &len);
    if (LBP16_SENDRECV_DEBUG)
        printf("RECV: %d bytes\n", recv);
    memcpy(buffer, local_buff, size);

    return 0;
}

int lbp16_hm2_read(u32 addr, void *buffer, int size) {
    if ((size/4) > LBP16_MAX_PACKET_DATA_SIZE) {
        printf("ERROR: LBP16: Requested %d units to read, but protocol supports up to %d units to be read per packet\n", size/4, LBP16_MAX_PACKET_DATA_SIZE);
        return -1;
    }

    return lbp16_read(CMD_READ_HOSTMOT2_ADDR32_INCR(size/4), addr, buffer, size);
}

int lbp16_hm2_write(u32 addr, void *buffer, int size) {
    return 0;
}

void lbp16_print_info() {
    lbp16_cmd_addr packet;
    int i, j;
    u8 idrom;
    char *mem_types[16] = {NULL, "registers", "memory", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "EEPROM", "flash"};
    char *mem_writeable[2] = {"RO", "RW"};
    char *acc_types[4] = {"8-bit", "16-bit", "32-bit", "64-bit"};
    char *led_debug_types[2] = {"Hostmot2", "eth debug"};
    char *boot_jumpers_types[4] = {"fixed "LBP16_HW_IP, "fixed from EEPROM", "from BOOTP", "INVALID"};
    lbp_mem_info_area mem_area;
    lbp_eth_eeprom_area eth_area;
    lbp_timers_area timers_area;
    lbp_status_area stat_area;
    lbp_info_area info_area;
    lbp16_cmd_addr cmds[LBP16_MEM_SPACE_COUNT];

    LBP16_INIT_PACKET4(cmds[0], CMD_READ_AREA_INFO_ADDR16_INCR(LBP_SPACE_HM2, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[1], CMD_READ_AREA_INFO_ADDR16_INCR(LBP_SPACE_ETH_CHIP, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[2], CMD_READ_AREA_INFO_ADDR16_INCR(LBP_SPACE_ETH_EEPROM, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[3], CMD_READ_AREA_INFO_ADDR16_INCR(LBP_SPACE_FPGA_FLASH, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[4], 0, 0);
    LBP16_INIT_PACKET4(cmds[5], 0, 0);
    LBP16_INIT_PACKET4(cmds[6], CMD_READ_AREA_INFO_ADDR16_INCR(LBP_SPACE_COMM_CTRL, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[7], CMD_READ_AREA_INFO_ADDR16_INCR(LBP_SPACE_BOARD_INFO, sizeof(mem_area)/2), 0);

    LBP16_INIT_PACKET4(packet, CMD_READ_ETH_EEPROM_ADDR16_INCR(sizeof(eth_area)/2), 0);
    memset(&eth_area, 0, sizeof(eth_area));
    sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    recvfrom(sd, (char*) &eth_area, sizeof(eth_area), 0, (struct sockaddr *) &client_addr, &len);

    LBP16_INIT_PACKET4(packet, CMD_READ_COMM_CTRL_ADDR16_INCR(sizeof(stat_area)/2), 0);
    memset(&stat_area, 0, sizeof(stat_area));
    sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    recvfrom(sd, (char*) &stat_area, sizeof(stat_area), 0, (struct sockaddr *) &client_addr, &len);

    LBP16_INIT_PACKET4(packet, CMD_READ_BOARD_INFO_ADDR16_INCR(sizeof(info_area)/2), 0);
    memset(&info_area, 0, sizeof(info_area));
    sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
    recvfrom(sd, (char*) &info_area, sizeof(info_area), 0, (struct sockaddr *) &client_addr, &len);

    if (info_area.LBP_version >= 3) {
        LBP16_INIT_PACKET4(packet, CMD_READ_TIMER_ADDR16_INCR(sizeof(timers_area)/2), 0);
        memset(&timers_area, 0, sizeof(timers_area));
        sendto(sd, (char*) &packet, sizeof(packet), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        recvfrom(sd, (char*) &timers_area, sizeof(timers_area), 0, (struct sockaddr *) &client_addr, &len);
    }

    printf("LBP16 firmware info:\n");
    printf("  memory spaces:\n");
    for (i = 0; i < LBP16_MEM_SPACE_COUNT; i++) {
        u32 size;

        if ((cmds[i].cmd_lo == 0) && (cmds[i].cmd_hi == 0)) continue;
        memset(&mem_area, 0, sizeof(mem_area));
        sendto(sd, (char*) &cmds[i], sizeof(cmds[i]), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
        recvfrom(sd, (char*) &mem_area, sizeof (mem_area), 0, (struct sockaddr *) &client_addr, &len);

        printf("    %d: %.*s (%s, %s", i, sizeof(mem_area.name), mem_area.name, mem_types[(mem_area.size  >> 8) & 0x7F],
          mem_writeable[(mem_area.size & 0x8000) >> 15]);
        for (j = 0; j < 4; j++) {
            if ((mem_area.size & 0xFF) & 1 << j)
                printf(", %s)", acc_types[j]);
        }
        size = pow(2, mem_area.range & 0x3F);
        if (size < 1024) {
            printf(" [size=%u]", size);
        } else if ((size >= 1024) && (size < 1024*1024)) {
            printf(" [size=%uK]", size/1024);
        } else if (size >= 1024*1024) {
            printf(" [size=%uM]", size/(1024*1024));
        }
        if (((mem_area.size  >> 8) & 0x7F) >= 0xE)
            printf(", page size: 0x%X, erase size: 0x%X",
              (unsigned int) pow(2, (mem_area.range >> 6) & 0x1F), (unsigned int) pow(2, (mem_area.range >> 11) & 0x1F));
        printf("\n");
    }
 
    printf("  [space 2] Ethernet eeprom:\n");
    printf("    mac address: %02X:%02X:%02X:%02X:%02X:%02X\n", HI_BYTE(eth_area.mac_addr_hi), LO_BYTE(eth_area.mac_addr_hi),
      HI_BYTE(eth_area.mac_addr_mid), LO_BYTE(eth_area.mac_addr_mid), HI_BYTE(eth_area.mac_addr_lo), LO_BYTE(eth_area.mac_addr_lo));
    printf("    ip address: %d.%d.%d.%d\n", HI_BYTE(eth_area.ip_addr_hi), LO_BYTE(eth_area.ip_addr_hi), HI_BYTE(eth_area.ip_addr_lo), LO_BYTE(eth_area.ip_addr_lo));
    printf("    board name: %.*s\n", sizeof(eth_area.name), eth_area.name);
    printf("    user leds: %s\n", led_debug_types[eth_area.led_debug & 0x1]);

    printf("  [space 3] FPGA flash eeprom:\n");
    lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &idrom, 4);
    printf("    flash id: 0x%02X %s\n", idrom, eeprom_get_idrom_type(idrom));

    if (info_area.LBP_version >= 3) {
        printf("  [space 4] timers:\n");
        printf("     uSTimeStampReg: 0x%04X\n", timers_area.uSTimeStampReg);
        printf("     WaituSReg: 0x%04X\n", timers_area.WaituSReg);
        printf("     HM2Timeout: 0x%04X\n", timers_area.HM2Timeout);
        printf("     WaitForHM2RefTime: 0x%04X\n", timers_area.WaitForHM2RefTime);
        printf("     WaitForHM2Timer1: 0x%04X\n", timers_area.WaitForHM2Timer1);
        printf("     WaitForHM2Timer2: 0x%04X\n", timers_area.WaitForHM2Timer2);
        printf("     WaitForHM2Timer3: 0x%04X\n", timers_area.WaitForHM2Timer3);
        printf("     WaitForHM2Timer4: 0x%04X\n",timers_area.WaitForHM2Timer4);
    }

    printf("  [space 6] LBP16 status/control:\n");
    printf("    packets recived: all %d, UDP %d, bad %d\n", stat_area.RXPacketCount, stat_area.RXUDPCount, stat_area.RXBadCount);
    printf("    packets sended: all %d, UDP %d, bad %d\n", stat_area.TXPacketCount, stat_area.TXUDPCount, stat_area.TXBadCount);
    printf("    parse errors: %d, mem errors %d, write errors %d\n", stat_area.LBPParseErrors, stat_area.LBPMemErrors, stat_area.LBPWriteErrors);
    printf("    debug LED ptr: 0x%04X\n", stat_area.DebugLEDPtr);
    printf("    scratch: 0x%04X\n", stat_area.Scratch);

    printf("  [space 7] LBP16 info:\n");
    printf("    board name: %.*s\n", sizeof(info_area.name), info_area.name);
    printf("    LBP16 version %d\n", info_area.LBP_version);
    printf("    firmware version %d\n", info_area.firmware_version);
    printf("    IP address jumpers at boot: %s\n", boot_jumpers_types[info_area.jumpers]);
}
