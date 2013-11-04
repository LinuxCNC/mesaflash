
#ifndef __COMMON_H
#define __COMMON_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#include "winio32/winio_nt.h"
typedef __int64 u64;

u8 inb(u32 addr);
void outb(u8 data, u32 addr);
u16 inw(u32 addr);
void outw(u16 data, u32 addr);
u32 inl(u32 addr);
void outl(u32 data, u32 addr);
void *map_memory(u32 base, u32 size);
#endif

#define LO_BYTE(x) ((x) & 0xFF)
#define HI_BYTE(x) (((x) & 0xFF00) >> 8)

void sleep_ns(u64 nanoseconds);

#endif
