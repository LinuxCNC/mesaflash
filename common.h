
#ifndef __COMMON_H
#define __COMMON_H

#ifdef __linux__
#include <pci/pci.h>
#elif _WIN32
#include "libpci/pci.h"
#include "winio32/winio.h"
typedef __int64 u64;

void init_io_library();
void release_io_library();
u8 inb(u32 addr);
void outb(u8 data, u32 addr);
u16 inw(u32 addr);
void outw(u16 data, u32 addr);
u32 inl(u32 addr);
void outl(u32 data, u32 addr);
void *map_memory(u32 base, u32 size, tagPhysStruct_t *phys);
void *unmap_memory(tagPhysStruct_t *phys);
#endif

#define LO_BYTE(x) ((x) & 0xFF)
#define HI_BYTE(x) (((x) & 0xFF00) >> 8)

void sleep_ns(u64 nanoseconds);

#endif
