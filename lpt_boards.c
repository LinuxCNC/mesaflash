
#include <pci/pci.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/ppdev.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "lpt_boards.h"

lpt_board_t lpt_boards[MAX_LPT_BOARDS];
int boards_count;

////////////////////////////////////////////////////////////////////////

static int parport_get(lpt_board_t *board, unsigned short base, unsigned short base_hi, unsigned int modes) {
    FILE *fd;
    char devname[64] = { 0, };
    char devpath[96] = { 0, };

    board->dev_fd = -1;
    board->base = base;
    board->base_hi = base_hi;

    if (base < 0x20) {
        /* base is the parallel port number. */
        snprintf(devname, sizeof(devname), "parport%u", base);
        goto found_dev;
    } else {
        char *buf = NULL;
        size_t bufsize = 0;

        /* base is the I/O port base. */
        fd = fopen("/proc/ioports", "r");
        if (!fd) {
            printf("Failed to open /proc/ioports: %s\n", strerror(errno));
            return -ENODEV;
        }
        while (getline(&buf, &bufsize, fd) > 0) {
            char *b = buf;
            unsigned int start, end;
            int count;

            while (b[0] == ' ' || b[0] == '\t') /* Strip leading whitespace */
                b++;
            count = sscanf(b, "%x-%x : %63s", &start, &end, devname);
            if (count != 3)
                continue;
//            if (strncmp(devname, "parport", 7) != 0)
//                continue;
            if (start == base) {
                fclose(fd);
                free(buf);
                goto found_dev;
            }
        }
        fclose(fd);
        free(buf);
    }
    printf("Did not find parallel port with base address 0x%03X\n", base);
    return -ENODEV;
found_dev:
    snprintf(devpath, sizeof(devpath), "/dev/%s", devname);

    printf("Using parallel port %s (base 0x%03X) with ioctl I/O\n", devpath, base);
    board->dev_fd = open(devpath, O_RDWR);
    if (board->dev_fd < 0) {
        printf("Failed to open parallel port %s: %s\n", devpath, strerror(errno));
        return -ENODEV;
    }
    if (ioctl(board->dev_fd, PPEXCL)) {
        printf("Failed to get exclusive access to parallel port %s\n", devpath);
        close(board->dev_fd);
        return -ENODEV;
    }
    if (ioctl(board->dev_fd, PPCLAIM)) {
        printf("Failed to claim parallel port %s\n", devpath);
        close(board->dev_fd);
        return -ENODEV;
    }
    return 0;
}

static void parport_release(lpt_board_t *board) {
    if (board->dev_fd < 0)
        return;
    ioctl(board->dev_fd, PPRELEASE);
    close(board->dev_fd);
}

void lpt_boards_init() {
}

void lpt_boards_scan() {
}
