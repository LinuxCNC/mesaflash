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
#elif _WIN32
#include "libpci/pci.h"
#endif

#include "anyio.h"

void spilbp_print_info() {
}

void spilbp_init(board_access_t *access) {
}

void spilbp_release() {
}
