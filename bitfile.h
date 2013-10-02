
#ifndef __BITFILE_H
#define __BITFILE_H

#include <pci/pci.h>
#include <stdio.h>

int print_bitfile_header(FILE *fp, char *part_name);
u8 bitfile_reverse_bits(u8 data);

#endif

