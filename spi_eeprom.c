
#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "spi_eeprom.h"
#include "spi_access_hm2.h"
#include "bitfile.h"
#include "pci_boards.h"

spi_eeprom_dev_t access;

u8 boot_block[BOOT_BLOCK_SIZE] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xAA, 0x99, 0x55, 0x66, 0x31, 0xE1,
    0xFF, 0xFF, 0x32, 0x61, 0x00, 0x00, 0x32, 0x81,
    0x0B, 0x08, 0x32, 0xA1, 0x00, 0x00, 0x32, 0xC1,
    0x0B, 0x01, 0x30, 0xA1, 0x00, 0x00, 0x33, 0x01,
    0x21, 0x00, 0x32, 0x01, 0x00, 0x1F, 0x30, 0xA1,
    0x00, 0x0E, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
    0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00
};

static u8 file_buffer[SECTOR_SIZE];

char *eeprom_get_flash_type(u8 flash_id) {
    switch (flash_id) {
        case ID_EEPROM_1M:  return "(size 1Mb)";
        case ID_EEPROM_2M:  return "(size 2Mb)";
        case ID_EEPROM_4M:  return "(size 4Mb)";
        case ID_EEPROM_8M:  return "(size 8Mb)";
        case ID_EEPROM_16M: return "(size 16Mb)";
        default:            return "(unknown size)";
    }
}

// modify MSB of boot block jmp address to user area
void prepare_boot_block(u8 flash_id) {
    switch (flash_id) {
        case ID_EEPROM_1M:  boot_block[25] = 0x01; break;
        case ID_EEPROM_2M:  boot_block[25] = 0x02; break;
        case ID_EEPROM_4M:  boot_block[25] = 0x04; break;
        case ID_EEPROM_8M:  boot_block[25] = 0x08; break;
        case ID_EEPROM_16M: boot_block[25] = 0x10; break;
    }
}

u32 eeprom_calc_user_space(u8 flash_id) {
    switch (flash_id) {
        case ID_EEPROM_1M:  return 0x10000; break;
        case ID_EEPROM_2M:  return 0x20000; break;
        case ID_EEPROM_4M:  return 0x40000; break;
        case ID_EEPROM_8M:  return 0x80000; break;
        case ID_EEPROM_16M: return 0x100000; break;
        default: return 0x80000; break;
    }
}

static void send_address(llio_t *self, u32 addr) {
    access.send_byte(self, (addr >> 16) & 0xFF);
    access.send_byte(self, (addr >> 8) & 0xFF);
    access.send_byte(self, addr & 0xFF);
}

static void write_enable(llio_t *self) {
    access.prefix(self);
    access.send_byte(self, SPI_CMD_WRITE_ENABLE);
    access.suffix(self);
}

static u8 read_status(llio_t *self) {
    u8 ret;

    access.prefix(self);
    access.send_byte(self, SPI_CMD_READ_STATUS);
    ret = access.recv_byte(self);
    access.suffix(self);
    return ret;
}

u8 read_flash_id(llio_t *self) {
    u8 ret;

    access.prefix(self);
    access.send_byte(self, SPI_CMD_READ_IDROM);
    send_address(self, 0); // three dummy bytes
    ret = access.recv_byte(self);
    access.suffix(self);
    return ret;
}

static void wait_for_write(llio_t *self) {
    u8 stat = read_status(self);

    while ((stat & WRITE_IN_PROGRESS_MASK) != 0) {
         stat = read_status(self);
    }
}

static u8 read_byte(llio_t *self, u32 addr) {
    u8 ret;

    access.prefix(self);
    access.send_byte(self, SPI_CMD_READ);
    send_address(self, addr);
    ret = access.recv_byte(self);
    access.suffix(self);
    return ret;
}

static void erase_sector(llio_t *self, u32 addr) {
    write_enable(self);
    access.prefix(self);
    access.send_byte(self, SPI_CMD_SECTOR_ERASE);
    send_address(self, addr);
    access.suffix(self);
    wait_for_write(self);
}

static void write_page(llio_t *self, u32 addr, char *buff, int buff_i) {
    int i;

    write_enable(self);
    access.prefix(self);
    access.send_byte(self, SPI_CMD_PAGE_WRITE);
    send_address(self, addr);// { note that add 0..7 should be 0}
    for (i = 0; i < PAGE_SIZE; i++) {
        access.send_byte(self, buff[buff_i + i]);
    }
    access.suffix(self);
    wait_for_write(self);
}

static int check_boot(llio_t *self) {
    int i;
    u8 data;

    for (i = 0; i < BOOT_BLOCK_SIZE; i++) {
        data = read_byte(self, i);
        if (data != boot_block[i]) {
            return -1;
        }
    }
    return 0;
}

static void write_boot(llio_t *self) {
    printf("Erasing sector 0 for boot block\n");
    erase_sector(self, BOOT_ADDRESS);
    memset(&file_buffer, 0, PAGE_SIZE);
    memcpy(&file_buffer, &boot_block, BOOT_BLOCK_SIZE);
    write_page(self, 0, (char *) &file_buffer, 0);
    printf("BootBlock installed\n");
}

static int start_programming(llio_t *self, u32 start_address, int fsize) {
    u32 sec_addr;
    int esectors, sector, max_sectors;
    board_t *board = self->private;

    access.set_cs_high(self);

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
        erase_sector(self, sec_addr);
        sec_addr = sec_addr + SECTOR_SIZE;
        printf("E");
        fflush(stdout);
    }
    printf("\n");
    access.set_cs_high(self);
    return 0;
}

static void done_programming(llio_t *self) {
    access.set_cs_high(self);
}

int eeprom_write_area(llio_t *self, char *bitfile_name, u32 start_address) {
    int bytesread, i;
    u32 eeprom_addr;
    char part_name[32];
    struct stat file_stat;
    u8 file_buffer[SECTOR_SIZE];
    FILE *fp;

    memset(&file_buffer, 0, sizeof(file_buffer));
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
    if (check_boot(self) == -1) {
        write_boot(self);
    } else {
        printf("Boot sector OK\n");
    }
    if (check_boot(self) == -1) {
        printf("Failed to write valid boot sector\n");
        fclose(fp);
        return -1;
    }

    if (start_programming(self, start_address, file_stat.st_size) == -1) {
        fclose(fp);
        return -1;
    }
    printf("Programming EEPROM area starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    eeprom_addr = start_address;
    while (!feof(fp)) {
      bytesread = fread(&file_buffer, 1, 8192, fp);
      i = 0;
      while (i < bytesread) {
        write_page(self, eeprom_addr, (char *) &file_buffer, i);
        i = i + PAGE_SIZE;
        eeprom_addr = eeprom_addr + PAGE_SIZE;
      }
      printf("W");
      fflush(stdout);
    }
    printf("\nBoard configuration updated successfully\n");
    done_programming(self);
    return 0;
}

int eeprom_verify_area(llio_t *self, char *bitfile_name, u32 start_address) {
    int bytesread, i, bindex;
    u8 rdata, mdata;
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
    if (check_boot(self) == -1) {
        printf("Error: BootSector is invalid\n");
        fclose(fp);
        return -1;
    } else {
        printf("Boot sector OK\n");
    }

    printf("Verifying EEPROM sectors starting from 0x%X...\n", (unsigned int) start_address);
    printf("  |");
    fflush(stdout);
    i = start_address;
    while (!feof(fp)) {
        bytesread = fread(&file_buffer, 1, 8192, fp);
        bindex = 0;
        while (bindex < bytesread) {
            rdata = read_byte(self, bindex + i);
            mdata = file_buffer[bindex];
            if (mdata != rdata) {
                printf("\nError at 0x%X expected: 0x%X but read: 0x%X\n", i+bindex, mdata, rdata);
                return -1;
            }
            bindex = bindex + 1;
        }
        i = i + 8192;
        printf("V");
        fflush(stdout);
    }

    fclose(fp);
    printf("\nBoard configuration verified successfully\n");
    return 0;
}

void eeprom_init(llio_t *self) {
    open_spi_access_hm2(self, &access);
}
