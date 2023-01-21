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
#include "types.h"
#include "bitfile.h"

// prints info about bitfile and returns header length or -1 when error
int print_bitfile_header(FILE *fp, char *part_name, int verbose_flag) {
    u8 buff[256];
    int sleng;
    int bytesread, conflength;
    int ret = 0;
    char str[80];
    char * partns;
    printf("Checking file... ");    
    bytesread = fread(&buff, 1, 14, fp);
    ret += bytesread;
    if (bytesread != 14) {
        if (feof(fp))
            printf("Error: Unexpected end of file\n");
        else
            printf("Error: IO error\n");
        return -1;
    }
    if ((buff[0] == 0) && (buff[1] == 9)) {
        printf("OK\n  File type: Xilinx bit file\n");
        if ((buff[11] == 0) && (buff[12] == 1) && (buff[13] == 0x61)) {
            bytesread = fread(&buff, 1, 2, fp);
            ret += bytesread;
            sleng = buff[0]*256 + buff[1];
            bytesread = fread(&buff, 1, sleng, fp);
            ret += bytesread;
            if (verbose_flag == 1) {
                printf("  Design name: %s\n", buff);
            }

            bytesread = fread(&buff, 1, 3, fp);
            ret += bytesread;
            sleng = buff[1]*256 + buff[2];
            bytesread = fread(&buff, 1, sleng, fp);
            ret += bytesread;
            if (verbose_flag == 1) {
                printf("  Part name: %s\n", buff);
            }
            strcpy(part_name, (char*) &buff);

            bytesread = fread(&buff, 1, 3, fp);
            ret += bytesread;
            sleng = buff[1]*256 + buff[2];
            bytesread = fread(&buff, 1, sleng, fp);
            ret += bytesread;
            if (verbose_flag == 1) {
                printf("  Design date: %s\n", buff);
            }

            bytesread = fread(&buff, 1, 3, fp);
            ret += bytesread;
            sleng = buff[1]*256 + buff[2];
            bytesread = fread(&buff, 1, sleng, fp);
            ret += bytesread;
            if (verbose_flag == 1) {
                printf("  Design time: %s\n", buff);
            }

            bytesread = fread(&buff, 1, 5, fp);
            ret += bytesread;
            conflength = buff[1]*16777216;
            conflength = conflength + buff[2]*65536;
            conflength = conflength + buff[3]*256;
            conflength = conflength + buff[4];
            if (verbose_flag == 1) {
                printf("  Config Length: %d\n", conflength);
            }
        }
        return ret;
    }
    rewind(fp);
    fgets(str, 20 ,fp); 
       ret += strlen(str);
    if (strncmp(str,"Version",7) == 0) {
        printf("OK\n  File type: Efinix bin file\n");
        if (verbose_flag == 1) {
            printf("  Compiler %s", str);
        }    
        // Compiler version
        fgets(str, 60, fp);
        ret += strlen(str);
        if (verbose_flag == 1) {
            printf("  %s", str);
        }    
       // Date
       fgets(str, 60, fp);
        ret += strlen(str);
        if (verbose_flag == 1) {
            printf("  %s", str);
        }
        // Project
        fgets(str, 80, fp);
        ret += strlen(str);
        fgets(str, 80, fp);
        ret += strlen(str);
        if (verbose_flag == 1) {
            printf("  %s", str);
        }
        // Family
        fgets(str, 60, fp);
        ret += strlen(str);
        if (verbose_flag == 1) {
            printf("  %s", str);
        }
        // Device
        fgets(str, 60, fp);
        ret += strlen(str);
        if (verbose_flag == 1) {
            printf("  %s", str);
        }
        partns = strtok(str," ");
        partns = strtok(NULL," ");
        partns[(strlen(partns)-1)] = '\0';
        strcpy(part_name, partns);
        // Configuration device width
        fgets(str, 60, fp);
        ret += strlen(str);
        if (verbose_flag == 1) {
            printf("  %s", str);
        }
        // Configuration mode
        fgets(str, 60, fp);
        ret += strlen(str);
        if (verbose_flag == 1) {
            printf("  %s", str);
        }
        // Padded bits
        fgets(str, 60, fp);
        ret += strlen(str);
        if (verbose_flag == 1) {
            printf("  %s", str);
        }
        
        return ret;
    }    
    if ((buff[0] == 0xFF) && (buff[1] == 0xFF) && (buff[2] == 0xFF) && (buff[3] == 0xFF)) {
        printf("Looks like a BIN file\n");
        return -1;
    }
    printf("Invalid bitfile header!\n");
    return -1;
}

//
// the fpga was originally designed to be programmed serially... even
// though we are doing it using a parallel interface, the bit ordering
// is based on the serial interface, and the data needs to be reversed
//

u8 bitfile_reverse_bits(u8 data) {
    static const u8 swaptab[256] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
    };
    return swaptab[data];
}
