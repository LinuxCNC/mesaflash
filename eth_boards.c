
#ifdef __linux__
#include <pci/pci.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>

#include "common.h"
#include "bitfile.h"
#include "anyio.h"
#include "eth_boards.h"
#include "lbp16.h"
#include "spi_eeprom.h"

extern board_t boards[MAX_BOARDS];
extern int boards_count;
static u8 page_buffer[PAGE_SIZE];
extern u8 boot_block[BOOT_BLOCK_SIZE];
static u8 file_buffer[SECTOR_SIZE];

#ifdef __linux__
int sd;
socklen_t len;
#elif _WIN32
SOCKET sd;
int len;
#endif
struct sockaddr_in server_addr, client_addr;

// eth access functions

inline int eth_socket_send_packet(void *packet, int size) {
    return sendto(sd, (char*) packet, size, 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
}

inline int eth_socket_recv_packet(void *buffer, int size) {
    return recvfrom(sd, (char*) buffer, size, 0, (struct sockaddr *) &client_addr, &len);
}

static void eth_socket_nonblocking() {
#ifdef __linux__
    int val = fcntl(sd, F_GETFL);

    val = val | O_NONBLOCK;
    fcntl(sd, F_SETFL, val);
#elif _WIN32
    u_long iMode = 1;

    ioctlsocket(sd, FIONBIO, &iMode);
#endif
}

static void eth_socket_blocking() {
#ifdef __linux__
    int val = fcntl(sd, F_GETFL);

    val = val & ~O_NONBLOCK;
    fcntl(sd, F_SETFL, val);
#elif _WIN32
    u_long iMode = 0;

    ioctlsocket(sd, FIONBIO, &iMode);
#endif
}

void eth_socket_set_dest_ip(char *addr_name) {
    server_addr.sin_addr.s_addr = inet_addr(addr_name);
}

static char *eth_socket_get_src_ip() {
    return inet_ntoa(client_addr.sin_addr);
}

static int lbp16_read(u16 cmd, u32 addr, void *buffer, int size) {
    lbp16_cmd_addr packet;
    int send, recv;
    u8 local_buff[size];

    LBP16_INIT_PACKET4(packet, cmd, addr);
    if (LBP16_SENDRECV_DEBUG)
        printf("SEND: %02X %02X %02X %02X | REQUEST %d bytes\n", packet.cmd_hi, packet.cmd_lo, packet.addr_hi, packet.addr_lo, size);
    send = eth_socket_send_packet(&packet, sizeof(packet));
    recv = eth_socket_recv_packet(&local_buff, sizeof(local_buff));
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
    send = eth_socket_send_packet(&packet, sizeof(lbp16_cmd_addr) + size);

    return 0;
}

int eth_read(llio_t *this, u32 addr, void *buffer, int size) {
    if ((size/4) > LBP16_MAX_PACKET_DATA_SIZE) {
        printf("ERROR: LBP16: Requested %d units to read, but protocol supports up to %d units to be read per packet\n", size/4, LBP16_MAX_PACKET_DATA_SIZE);
        return -1;
    }

    return lbp16_read(CMD_READ_HOSTMOT2_ADDR32_INCR(size/4), addr, buffer, size);
}

int eth_write(llio_t *this, u32 addr, void *buffer, int size) {
    if ((size/4) > LBP16_MAX_PACKET_DATA_SIZE) {
        printf("ERROR: LBP16: Requested %d units to write, but protocol supports up to %d units to be write per packet\n", size/4, LBP16_MAX_PACKET_DATA_SIZE);
        return -1;
    }

    return lbp16_write(CMD_WRITE_HOSTMOT2_ADDR32_INCR(size/4), addr, buffer, size);
}

// flash functions

static void page_read(u32 addr, void *buff) {
    lbp16_cmd_addr_data32 write_addr_pck;
    lbp16_cmd_addr read_page_pck;
    int send, recv;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    LBP16_INIT_PACKET4(read_page_pck, CMD_READ_FPGA_FLASH_ADDR32(64), FLASH_DATA_REG);

    send = eth_socket_send_packet(&write_addr_pck, sizeof(write_addr_pck));
    send = eth_socket_send_packet(&read_page_pck, sizeof(read_page_pck));
    recv = eth_socket_recv_packet(buff, PAGE_SIZE);
}

static void sector_erase(u32 addr) {
    lbp16_erase_flash_sector_packets sector_erase_pck;
    lbp16_cmd_addr_data32 write_addr_pck;
    int send, recv;
    u32 ignored;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    send = eth_socket_send_packet(&write_addr_pck, sizeof(write_addr_pck));
    if (send < 0)
        printf("ERROR: %s(): %s\n", __func__, strerror(errno));

    LBP16_INIT_PACKET6(sector_erase_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A03);
    LBP16_INIT_PACKET8(sector_erase_pck.fl_erase_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_SEC_ERASE_REG, 0);
    send = eth_socket_send_packet(&sector_erase_pck, sizeof(sector_erase_pck));
    if (send < 0)
        printf("ERROR: %s(): %s\n", __func__, strerror(errno));
    // packet read for board syncing
    recv = lbp16_read(CMD_READ_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, &ignored, 4);
}

static void page_write(void *buff) {
    lbp16_write_flash_page_packets write_page_pck;
    int send, recv;
    u32 ignored;

    LBP16_INIT_PACKET6(write_page_pck.write_ena_pck, CMD_WRITE_COMM_CTRL_ADDR16(1), COMM_CTRL_WRITE_ENA_REG, 0x5A03);
    LBP16_INIT_PACKET4(write_page_pck.fl_write_page_pck, CMD_WRITE_FPGA_FLASH_ADDR32(64), FLASH_DATA_REG);
    memcpy(&write_page_pck.fl_write_page_pck.page, buff, 256);
    send = eth_socket_send_packet(&write_page_pck, sizeof(write_page_pck));
    // packet read for board syncing
    recv = lbp16_read(CMD_READ_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, &ignored, 4);
}

static int check_boot() {
    int i;

    page_read(0x0, &page_buffer);
    for (i = 0; i < BOOT_BLOCK_SIZE; i++) {
        if (boot_block[i] != page_buffer[i]) {
            return -1;
        }
    }
    return 0;
}

static void write_boot() {
    printf("Erasing sector 0 for boot block\n");
    sector_erase(BOOT_ADDRESS);
    memset(&file_buffer, 0, PAGE_SIZE);
    memcpy(&file_buffer, &boot_block, BOOT_BLOCK_SIZE);
    page_write(file_buffer);
    printf("BootBlock installed\n");
}

static void write_flash_address(u32 addr) {
    lbp16_cmd_addr_data32 write_addr_pck;
    int send;

    LBP16_INIT_PACKET8(write_addr_pck, CMD_WRITE_FPGA_FLASH_ADDR32(1), FLASH_ADDR_REG, addr);
    send = eth_socket_send_packet(&write_addr_pck, sizeof(write_addr_pck));
}

static int start_programming(llio_t *self, u32 start_address, int fsize) {
    board_t *board = self->private;
    u32 sec_addr;
    int esectors, sector, max_sectors;

    esectors = (fsize - 1) / SECTOR_SIZE;
    if (start_address == FALLBACK_ADDRESS) {
        max_sectors = eeprom_calc_user_space(board->flash_id) / SECTOR_SIZE - 1;
    } else {
        max_sectors = eeprom_calc_user_space(board->flash_id) / SECTOR_SIZE;
    }
    if (esectors > max_sectors) {
        printf("File Size too large to fit\n");
        return -1;
    }
    printf("EEPROM sectors to write: %d, max sectors in area: %d\n", esectors, max_sectors);
    sec_addr = start_address;
    printf("Erasing EEPROM sectors starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    for (sector = 0; sector <= esectors; sector++) {
        sector_erase(sec_addr);
        sec_addr = sec_addr + SECTOR_SIZE;
        printf("E");
        fflush(stdout);
    }
    printf("\n");
    return 0;
}

// public functions

int eth_program_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    int bytesread, i, bindex;
    char part_name[32];
    struct stat file_stat;
    FILE *fp;

    if (stat(bitfile_name, &file_stat) != 0) {
        printf("Can't find file %s\n", bitfile_name);
        return -1;
    }

    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return -1;
    }
    if (print_bitfile_header(fp, (char*) &part_name) == -1) {
        fclose(fp);
        return -1;
    }
/*    if (strcmp(part_name, active_board.fpga_part_number) != 0) {
        printf("Error: wrong bitfile destination device: %s, should be %s\n", part_name, active_board.fpga_part_number);
        fclose(fp);
        return -1;
    }*/
    if (check_boot() == -1) {
        write_boot();
    } else {
        printf("Boot sector OK\n");
    }
    if (check_boot() == -1) {
        printf("Failed to write valid boot sector\n");
        fclose(fp);
        return -1;
    }

    if (start_programming(self, start_address, file_stat.st_size) == -1) {
        fclose(fp);
        return -1;
    }
    printf("Programming EEPROM sectors starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    i = start_address;
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            write_flash_address(i);
            page_write(&file_buffer[bindex]);
            i += PAGE_SIZE;
            bindex += PAGE_SIZE;
        }
        printf("W");
        fflush(stdout);
    }

    fclose(fp);
    printf("\nBoard configuration verified successfully\n");
    return 0;
}

int eth_verify_flash(llio_t *self, char *bitfile_name, u32 start_address) {
    int bytesread, fl_addr, bindex;
    char part_name[32];
    struct stat file_stat;
    FILE *fp;
    int i;

    if (stat(bitfile_name, &file_stat) != 0) {
        printf("Can't find file %s\n", bitfile_name);
        return -1;
    }

    fp = fopen(bitfile_name, "rb");
    if (fp == NULL) {
        printf("Can't open file %s: %s\n", bitfile_name, strerror(errno));
        return -1;
    }
    if (print_bitfile_header(fp, (char*) &part_name) == -1) {
        fclose(fp);
        return -1;
    }
/*    if (strcmp(part_name, active_board.fpga_part_number) != 0) {
        printf("Error: wrong bitfile destination device: %s, should be %s\n", part_name, active_board.fpga_part_number);
        fclose(fp);
        return -1;
    }*/
    if (check_boot() == -1) {
        printf("Error: BootSector is invalid\n");
        fclose(fp);
        return -1;
    } else {
        printf("Boot sector OK\n");
    }

    printf("Verifying EEPROM sectors starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    fl_addr = start_address;
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            page_read(fl_addr, &page_buffer);
            for (i = 0; i < PAGE_SIZE; i++, bindex++) {
                if (file_buffer[bindex] != page_buffer[i]) {
                   printf("\nError at 0x%X expected: 0x%X but read: 0x%X\n", bindex, file_buffer[bindex], page_buffer[i]);
                   return -1;
                }
            }
            fl_addr += PAGE_SIZE;
        }
        printf("V");
        fflush(stdout);
    }

    fclose(fp);
    printf("\nBoard configuration verified successfully\n");
    return 0;
}

int eth_boards_init(board_access_t *access) {
// open socket
#ifdef __linux__
    sd = socket (PF_INET, SOCK_DGRAM, 0);
#elif _WIN32
    int iResult;
    WSADATA wsaData;
    u_long iMode = 1;    // block mode in windows

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return;
    }

    sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sd == 0) {
        printf("Can't open socket: %d %d\n", sd, errno);
        return;
    }
#endif
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LBP16_UDP_PORT);
    len = sizeof(client_addr);
    return 0;
}

void eth_boards_cleanup(board_access_t *access) {
    close(sd);
}

void eth_boards_scan(board_access_t *access) {
    lbp16_cmd_addr packet, packet2;
    char addr[16];
    int i;
    int send = 0, recv = 0;
    u32 cookie;
    char *ptr;

    eth_socket_nonblocking();

    strncpy(addr, access->dev_addr, 16);
    ptr = strrchr(addr, '.');
    *ptr = '\0';

    LBP16_INIT_PACKET4(packet, CMD_READ_HM2_COOKIE, HM2_COOKIE_REG);
    for (i = 1; i < 256; i++) {
        char addr_name[32];

        sprintf(addr_name, "%s.%d", addr, i);
        eth_socket_set_dest_ip(addr_name);
        send = eth_socket_send_packet(&packet, sizeof(packet));
        sleep_ns(2*1000*1000);
        recv = eth_socket_recv_packet(&cookie, sizeof(cookie));

        if ((recv > 0) && (cookie == HM2_COOKIE)) {
            char buff[20];
            board_t *board = &boards[boards_count];

            eth_socket_blocking();
            LBP16_INIT_PACKET4(packet2, CMD_READ_BOARD_INFO_ADDR16_INCR(8), 0);
            memset(buff, 0, sizeof(buff));
            send = eth_socket_send_packet(&packet2, sizeof(packet2));
            recv = eth_socket_recv_packet(&buff, sizeof(buff));

            if (strncmp(buff, "7I80DB-16", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 4;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "J2";
                board->llio.ioport_connector_name[1] = "J3";
                board->llio.ioport_connector_name[2] = "J4";
                board->llio.ioport_connector_name[3] = "J5";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else if (strncmp(buff, "7I80DB-25", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 4;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "J2";
                board->llio.ioport_connector_name[1] = "J3";
                board->llio.ioport_connector_name[2] = "J4";
                board->llio.ioport_connector_name[3] = "J5";
                board->llio.fpga_part_number = "6slx25ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else if (strncmp(buff, "7I80HD-16", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.ioport_connector_name[2] = "P3";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else if (strncmp(buff, "7I80HD-25", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 24;
                board->llio.ioport_connector_name[0] = "P1";
                board->llio.ioport_connector_name[1] = "P2";
                board->llio.ioport_connector_name[2] = "P3";
                board->llio.fpga_part_number = "6slx25ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else if (strncmp(buff, "7I76E-16", 9) == 0) {
                board->type = BOARD_ETH;
                strncpy(board->dev_addr, eth_socket_get_src_ip(), 16);
                strncpy(board->llio.board_name, buff, 16);
                board->llio.num_ioport_connectors = 3;
                board->llio.pins_per_connector = 17;
                board->llio.ioport_connector_name[0] = "on-card";
                board->llio.ioport_connector_name[1] = "P1";
                board->llio.ioport_connector_name[2] = "P2";
                board->llio.fpga_part_number = "6slx16ftg256";
                board->llio.num_leds = 4;
                board->llio.read = &eth_read;
                board->llio.write = &eth_write;
                board->llio.program_flash = &eth_program_flash;
                board->llio.verify_flash = &eth_verify_flash;
                board->llio.private = board;
                lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &(board->flash_id), 4);
                prepare_boot_block(board->flash_id);
                board->flash_start_address = eeprom_calc_user_space(board->flash_id);
                board->llio.verbose = access->verbose;
            } else {
                printf("Unsupported ethernet device %s at %s\n", buff, eth_socket_get_src_ip());
                continue;
            }
            boards_count++;

            eth_socket_nonblocking();
        }
   }
   eth_socket_blocking();
}

void eth_print_info(board_t *board) {
    lbp16_cmd_addr packet;
    int i, j;
    u32 flash_id;
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

    printf("\nETH device %s at ip=%s\n", board->llio.board_name, eth_socket_get_src_ip());
    if (board->llio.verbose == 0)
        return;
        
    LBP16_INIT_PACKET4(cmds[0], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_HM2, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[1], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_ETH_CHIP, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[2], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_ETH_EEPROM, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[3], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_FPGA_FLASH, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[4], 0, 0);
    LBP16_INIT_PACKET4(cmds[5], 0, 0);
    LBP16_INIT_PACKET4(cmds[6], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_COMM_CTRL, sizeof(mem_area)/2), 0);
    LBP16_INIT_PACKET4(cmds[7], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_BOARD_INFO, sizeof(mem_area)/2), 0);

    LBP16_INIT_PACKET4(packet, CMD_READ_ETH_EEPROM_ADDR16_INCR(sizeof(eth_area)/2), 0);
    memset(&eth_area, 0, sizeof(eth_area));
    eth_socket_send_packet(&packet, sizeof(packet));
    eth_socket_recv_packet(&eth_area, sizeof(eth_area));

    LBP16_INIT_PACKET4(packet, CMD_READ_COMM_CTRL_ADDR16_INCR(sizeof(stat_area)/2), 0);
    memset(&stat_area, 0, sizeof(stat_area));
    eth_socket_send_packet(&packet, sizeof(packet));
    eth_socket_recv_packet(&stat_area, sizeof(stat_area));

    LBP16_INIT_PACKET4(packet, CMD_READ_BOARD_INFO_ADDR16_INCR(sizeof(info_area)/2), 0);
    memset(&info_area, 0, sizeof(info_area));
    eth_socket_send_packet(&packet, sizeof(packet));
    eth_socket_recv_packet(&info_area, sizeof(info_area));

    if (info_area.LBP16_version >= 3) {
        LBP16_INIT_PACKET4(cmds[4], CMD_READ_AREA_INFO_ADDR16_INCR(LBP16_SPACE_TIMER, sizeof(mem_area)/2), 0);
        LBP16_INIT_PACKET4(packet, CMD_READ_TIMER_ADDR16_INCR(sizeof(timers_area)/2), 0);
        memset(&timers_area, 0, sizeof(timers_area));
        eth_socket_send_packet(&packet, sizeof(packet));
        eth_socket_recv_packet(&timers_area, sizeof(timers_area));
    }

    printf("LBP16 firmware info:\n");
    printf("  memory spaces:\n");
    for (i = 0; i < LBP16_MEM_SPACE_COUNT; i++) {
        u32 size;

        if ((cmds[i].cmd_lo == 0) && (cmds[i].cmd_hi == 0)) continue;
        memset(&mem_area, 0, sizeof(mem_area));
        eth_socket_send_packet(&cmds[i], sizeof(cmds[i]));
        eth_socket_recv_packet(&mem_area, sizeof (mem_area));

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
 
    printf("  [space 0] Hostmot2\n");
    printf("  [space 2] Ethernet eeprom:\n");
    printf("    mac address: %02X:%02X:%02X:%02X:%02X:%02X\n", HI_BYTE(eth_area.mac_addr_hi), LO_BYTE(eth_area.mac_addr_hi),
      HI_BYTE(eth_area.mac_addr_mid), LO_BYTE(eth_area.mac_addr_mid), HI_BYTE(eth_area.mac_addr_lo), LO_BYTE(eth_area.mac_addr_lo));
    printf("    ip address: %d.%d.%d.%d\n", HI_BYTE(eth_area.ip_addr_hi), LO_BYTE(eth_area.ip_addr_hi), HI_BYTE(eth_area.ip_addr_lo), LO_BYTE(eth_area.ip_addr_lo));
    printf("    board name: %.*s\n", sizeof(eth_area.name), eth_area.name);
    printf("    user leds: %s\n", led_debug_types[eth_area.led_debug & 0x1]);

    printf("  [space 3] FPGA flash eeprom:\n");
    lbp16_read(CMD_READ_FLASH_IDROM, FLASH_ID_REG, &flash_id, 4);
    printf("    flash id: 0x%02X %s\n", flash_id, eeprom_get_flash_type(flash_id));

    if (info_area.LBP16_version >= 3) {
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
    printf("    LBP16 version %d\n", info_area.LBP16_version);
    printf("    firmware version %d\n", info_area.firmware_version);
    printf("    IP address jumpers at boot: %s\n", boot_jumpers_types[info_area.jumpers]);
}
