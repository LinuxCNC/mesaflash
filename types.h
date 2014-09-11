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

#ifndef __TYPES_H
#define __TYPES_H

#include <sys/types.h>

#ifdef __linux__
typedef u_int8_t u8;
typedef u_int16_t u16;
typedef u_int32_t u32;
#elif _WIN32
#include <windef.h>
typedef BYTE u8;
typedef WORD u16;
typedef DWORD u32;
#endif

#include <limits.h>
#if ULONG_MAX > 0xffffffff
typedef unsigned long u64;
#else
typedef unsigned long long u64;
#endif

#endif
