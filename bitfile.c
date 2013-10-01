
#include <pci/pci.h>
#include <stdio.h>
#include <string.h>

#include "bitfile.h"

// prints info about bitfile and returns header length or -1 when error
int print_bitfile_header(FILE *fp, char *part_name) {
    u8 buff[256];
    int sleng;
    int bytesread, conflength;
    int ret = 0;

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
        printf("OK\n  File type: BIT file\n");
        if ((buff[11] == 0) && (buff[12] == 1) && (buff[13] == 0x61)) {
            bytesread = fread(&buff, 1, 2, fp);
            ret += bytesread;
            sleng = buff[0]*256 + buff[1];
            bytesread = fread(&buff, 1, sleng, fp);
            ret += bytesread;
            printf("  Design name: %s\n", buff);

            bytesread = fread(&buff, 1, 3, fp);
            ret += bytesread;
            sleng = buff[1]*256 + buff[2];
            bytesread = fread(&buff, 1, sleng, fp);
            ret += bytesread;
            printf("  Part name: %s\n", buff);
            strcpy(part_name, (char*) &buff);

            bytesread = fread(&buff, 1, 3, fp);
            ret += bytesread;
            sleng = buff[1]*256 + buff[2];
            bytesread = fread(&buff, 1, sleng, fp);
            ret += bytesread;
            printf("  Design date: %s\n", buff);

            bytesread = fread(&buff, 1, 3, fp);
            ret += bytesread;
            sleng = buff[1]*256 + buff[2];
            bytesread = fread(&buff, 1, sleng, fp);
            ret += bytesread;
            printf("  Design time: %s\n", buff);

            bytesread = fread(&buff, 1, 5, fp);
            ret += bytesread;
            conflength = buff[1]*16777216;
            conflength = conflength + buff[2]*65536;
            conflength = conflength + buff[3]*256;
            conflength = conflength + buff[4];
            printf("  Config Length: %d\n", conflength);
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

