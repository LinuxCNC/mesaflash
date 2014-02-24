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

#ifdef __linux__
#include <pci/pci.h>
#include <time.h>
#elif _WIN32
#include <windows.h>
#include "libpci/pci.h"
#endif
#include <stdio.h>

#include "common.h"

#ifdef _WIN32
void init_io_library() {
    InitializeWinIo();
}

void release_io_library() {
    ShutdownWinIo();
}

u8 inb(u32 addr) {
    u32 val;
    GetPortVal((WORD) addr, &val, 1);
    return val & 0xFF;
}

void outb(u8 data, u32 addr) {
    SetPortVal((WORD) addr, (DWORD) data, 1);
}

u16 inw(u32 addr) {
    u32 val;
    GetPortVal((WORD) addr, &val, 2);
    return val & 0xFFFF;
}

void outw(u16 data, u32 addr) {
    SetPortVal((WORD) addr, (DWORD) data, 2);
}

u32 inl(u32 addr) {
    u32 val;
    GetPortVal((WORD) addr, &val, 4);
    return val;
}

void outl(u32 data, u32 addr) {
    SetPortVal((WORD) addr, (DWORD) data, 4);
}

void *map_memory(u32 base, u32 size, tagPhysStruct_t *phys) {
    void *ptr;

    memset(phys, 0, sizeof(tagPhysStruct_t));
    phys->pvPhysAddress = (DWORD64)(DWORD32) base;
    phys->dwPhysMemSizeInBytes = size;

    ptr = MapPhysToLin(phys);
    return ptr;
}

void *unmap_memory(tagPhysStruct_t *phys) {
    UnmapPhysicalMemory(phys);
}
#endif

void sleep_ns(u64 nanoseconds) {
#ifdef __linux__
    struct timespec tv, tvret;

    tv.tv_sec = 0;
    tv.tv_nsec = nanoseconds;
    nanosleep(&tv, &tvret);
#elif _WIN32
    Sleep(nanoseconds/1000/1000);
#endif
}

