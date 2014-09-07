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

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "anyio.h"
#include "serial_boards.h"
#include "lbp16.h"
#include "eeprom.h"

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <math.h>

extern board_t boards[MAX_BOARDS];
extern int boards_count;

int sd;

// serial access functions

inline int serial_send_packet(void *packet, int size) {
    int send = write(sd, packet, size);

    sleep_ns((size + 100)*100000);
    return send;
}

inline int serial_recv_packet(void *buffer, int size) {
    int recv = read(sd, buffer, size);

    return recv;
}

static int lbp16_read(u16 cmd, u32 addr, void *buffer, int size) {
    lbp16_cmd_addr packet;
    int send, recv;
    u8 local_buff[size];

    LBP16_INIT_PACKET4(packet, cmd, addr);
    if (LBP16_SENDRECV_DEBUG)
        printf("SEND: %02X %02X %02X %02X | REQUEST %d bytes\n", packet.cmd_hi, packet.cmd_lo, packet.addr_hi, packet.addr_lo, size);
    send = serial_send_packet(&packet, sizeof(packet));
    recv = serial_recv_packet(&local_buff, sizeof(local_buff));
    if (LBP16_SENDRECV_DEBUG)
        printf("RECV: %d bytes\n", recv);
    memcpy(buffer, local_buff, size);

    return 0;
}

static int lbp16_write(u16 cmd, u32 addr, void *buffer, int size) {
    static struct {
        lbp16_cmd_addr wr_packet;
        u8 tmp_buffer[127*8];
    } packet;
    int send;

    LBP16_INIT_PACKET4(packet.wr_packet, cmd, addr);
    memcpy(&packet.tmp_buffer, buffer, size);
    if (LBP16_SENDRECV_DEBUG)
        printf("SEND: %02X %02X %02X %02X | WRITE %d bytes\n", packet.wr_packet.cmd_hi, packet.wr_packet.cmd_lo,
          packet.wr_packet.addr_hi, packet.wr_packet.addr_lo, size);
    send = serial_send_packet(&packet, sizeof(lbp16_cmd_addr) + size);

    return 0;
}

static int serial_read(llio_t *self, u32 addr, void *buffer, int size) {
    return lbp16_read(CMD_READ_HOSTMOT2_ADDR32_INCR(size/4), addr, buffer, size);
}

static int serial_write(llio_t *self, u32 addr, void *buffer, int size) {
    return lbp16_write(CMD_WRITE_HOSTMOT2_ADDR32_INCR(size/4), addr, buffer, size);
}

static int serial_board_open(board_t *board) {
    lbp16_cmd_addr packet;
    u32 flash_id;

//    eeprom_init(&(board->llio));
    LBP16_INIT_PACKET4(packet, CMD_READ_FLASH_IDROM, FLASH_ID_REG);
    //serial_send_packet(&packet, sizeof(packet));
    //serial_recv_packet(&flash_id, 4);
    board->flash_id = 0;
    if (board->fallback_support == 1) {
        eeprom_prepare_boot_block(board->flash_id);
        board->flash_start_address = eeprom_calc_user_space(board->flash_id);
    } else {
        board->flash_start_address = 0;
    }
    return 0;
}

static int serial_board_close(board_t *board) {
    return 0;
}

// public functions

int serial_boards_init(board_access_t *access) {
#ifdef __linux__
    struct termios options;

    sd = open(access->dev_addr, O_RDWR | O_NOCTTY | O_NDELAY);
    if (sd == -1) {
        perror("Unable to open the serial port\n");
    }

    tcgetattr(sd, &options);        /* Get the current options for the port */
    cfsetispeed(&options, B115200);                 /* Set the baud rates to 9600 */
    cfsetospeed(&options, B115200);    

    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    
    
    options.c_lflag = 0;    
       
    options.c_cflag &= ~CRTSCTS;          /* Disable hardware flow control */  
    options.c_iflag &= ~IGNBRK;
    
    options.c_oflag = 0;
 
    tcsetattr(sd, TCSANOW, &options);
#elif _WIN32
    char dev_name[32];
    
    snprintf(dev_name, 32, "\\\\.\\%s", access->dev_addr);
    sd = CreateFile(dev_name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (sd == INVALID_HANDLE_VALUE) { 
        printf("Unable to open the serial port %d\n", errno);
	}
#endif
    return 0;
}

void serial_boards_cleanup(board_access_t *access) {
    close(sd);
}

void serial_boards_scan(board_access_t *access) {
    lbp16_cmd_addr packet;
    int send = 0, recv = 0, ret = 0;
    u8 buff[16];

    LBP16_INIT_PACKET4(packet, CMD_READ_BOARD_INFO_ADDR16_INCR(8), 0);
    send = serial_send_packet(&packet, sizeof(packet));
    recv = serial_recv_packet(buff, 16);
    if (strncmp(buff, "7I90HD", 6) == 0) {
        board_t *board = &boards[boards_count];

        board->type = BOARD_SER;
        strncpy(board->dev_addr, access->dev_addr, 16);
        strncpy(board->llio.board_name, buff, 16);
        board->llio.num_ioport_connectors = 3;
        board->llio.pins_per_connector = 24;
        board->llio.ioport_connector_name[0] = "P1";
        board->llio.ioport_connector_name[1] = "P2";
        board->llio.ioport_connector_name[2] = "P3";
        board->llio.fpga_part_number = "6slx9tqg144";
        board->llio.num_leds = 2;
        board->llio.read = &serial_read;
        board->llio.write = &serial_write;
        board->llio.private = board;

        board->open = &serial_board_open;
        board->close = &serial_board_close;
        board->print_info = &serial_print_info;
        board->flash = BOARD_FLASH_REMOTE;
        board->fallback_support = 1;
        board->llio.verbose = access->verbose;
        
        boards_count++;
    }
}

void serial_print_info(board_t *board) {
    lbp16_cmd_addr packet;
    int i, j, recv;
    u32 flash_id;
    char *mem_types[16] = {NULL, "registers", "memory", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "EEPROM", "flash"};
    char *mem_writeable[2] = {"RO", "RW"};
    char *acc_types[4] = {"8-bit", "16-bit", "32-bit", "64-bit"};
    char *led_debug_types[2] = {"Hostmot2", "eth debug"};
    char *boot_jumpers_types[4] = {"fixed "LBP16_HW_IP, "fixed from EEPROM", "from BOOTP", "INVALID"};
    lbp_mem_info_area mem_area;
    lbp_timers_area timers_area;
    lbp_status_area stat_area;
    lbp_info_area info_area;
    lbp16_cmd_addr cmds[LBP16_MEM_SPACE_COUNT];

    printf("\nSerial device %s at %s\n", board->llio.board_name, board->dev_addr);
    if (board->llio.verbose == 0)
        return;
        
    LBP16_INIT_PACKET4(cmds[0], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_HM2, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[1], 0, 0);
    LBP16_INIT_PACKET4(cmds[2], 0, 0);
    LBP16_INIT_PACKET4(cmds[3], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_FPGA_FLASH, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[4], 0, 0);
    LBP16_INIT_PACKET4(cmds[5], 0, 0);
    LBP16_INIT_PACKET4(cmds[6], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_COMM_CTRL, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[7], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_BOARD_INFO, sizeof(mem_area)/2), 0);

    LBP16_INIT_PACKET4(packet, CMD_READ_COMM_CTRL_ADDR16_INCR(sizeof(stat_area)/2), 0);
    memset(&stat_area, 0, sizeof(stat_area));
    serial_send_packet(&packet, sizeof(packet));
    recv = serial_recv_packet(&stat_area, sizeof(stat_area));

    LBP16_INIT_PACKET4(packet, CMD_READ_BOARD_INFO_ADDR16_INCR(sizeof(info_area)/2), 0);
    memset(&info_area, 0, sizeof(info_area));
    serial_send_packet(&packet, sizeof(packet));
    recv = serial_recv_packet(&info_area, sizeof(info_area));

    if (info_area.LBP16_version >= 3) {
        LBP16_INIT_PACKET4(cmds[4], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_TIMER, sizeof(mem_area)/2), 0);
        LBP16_INIT_PACKET4(packet, CMD_READ_TIMER_ADDR16_INCR(sizeof(timers_area)/2), 0);
        memset(&timers_area, 0, sizeof(timers_area));
        serial_send_packet(&packet, sizeof(packet));
        recv = serial_recv_packet(&timers_area, sizeof(timers_area));
    }

    printf("Communication:\n");
    printf("  transport layer: serial\n");
    printf("  protocol: LBP16 version %d\n", info_area.LBP16_version);

    show_board_info(board);

    printf("Board firmware info:\n");
    printf("  memory spaces:\n");
    for (i = 0; i < LBP16_MEM_SPACE_COUNT; i++) {
        u32 size;

        if ((cmds[i].cmd_lo == 0) && (cmds[i].cmd_hi == 0)) continue;
        memset(&mem_area, 0, sizeof(mem_area));
        serial_send_packet(&cmds[i], sizeof(cmds[i]));
        serial_recv_packet(&mem_area, sizeof (mem_area));

        printf("    %d: %.*s (%s, %s", i, sizeof(mem_area.name), mem_area.name, mem_types[(mem_area.size  >> 8) & 0x7F],
          mem_writeable[(mem_area.size & 0x8000) >> 15]);
        for (j = 0; j < 4; j++) {
            if ((mem_area.size & 0xFF) & 1 << j)
                printf(", %s)", acc_types[j]);
        }
        size = pow(2, mem_area.range & 0x3F);
        show_formatted_size(size);
        if (((mem_area.size  >> 8) & 0x7F) >= 0xE)
            printf(", page size: %u, erase size: %u",
              (unsigned int) pow(2, (mem_area.range >> 6) & 0x1F), (unsigned int) pow(2, (mem_area.range >> 11) & 0x1F));
        printf("\n");
    }
 
    printf("  [space 0] HostMot2\n");
    printf("  [space 3] FPGA flash eeprom:\n");
    lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &flash_id, 4);
    printf("    flash size: %s (id: 0x%02X)\n", eeprom_get_flash_type(flash_id), flash_id);

    if (info_area.LBP16_version >= 3) {
        printf("  [space 4] timers:\n");
        printf("    uSTimeStampReg: 0x%04X\n", timers_area.uSTimeStampReg);
        printf("    WaituSReg: 0x%04X\n", timers_area.WaituSReg);
        printf("    HM2Timeout: 0x%04X\n", timers_area.HM2Timeout);
    }

    printf("  [space 6] LBP16 control/status:\n");
    printf("    packets received: all %d, good %d, bad %d\n", stat_area.RXPacketCount, stat_area.RXGoodCount, stat_area.RXBadCount);
    printf("    packets sended: all %d, good %d, bad %d\n", stat_area.TXPacketCount, stat_area.TXGoodCount, stat_area.TXBadCount);
    printf("    parse errors: %d, mem errors %d, write errors %d\n", stat_area.LBPParseErrors, stat_area.LBPMemErrors, stat_area.LBPWriteErrors);
    printf("    error flags: 0x%04X\n", stat_area.ErrorReg);
    printf("    debug LED ptr: 0x%04X\n", stat_area.DebugLEDPtr);
    printf("    scratch: 0x%04X\n", stat_area.Scratch);

    printf("  [space 7] LBP16 info:\n");
    printf("    board name: %.*s\n", sizeof(info_area.name), info_area.name);
    printf("    LBP16 protocol version %d\n", info_area.LBP16_version);
    printf("    board firmware version %d\n", info_area.firmware_version);
}
