
#ifndef __BITFILE_H
#define __BITFILE_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#endif
#include <stdio.h>

int print_bitfile_header(FILE *fp, char *part_name);
u8 bitfile_reverse_bits(u8 data);

#endif

