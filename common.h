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
