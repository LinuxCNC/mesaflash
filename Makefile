#
#    Copyright (C) 2013-2014 Michael Geszkiewicz
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
#

# target (linux, windows)
TARGET = linux

CC = gcc
RM = rm -f
MATHLIB = -lm
OPT = -O0

#DEBUG = -g -pedantic -Wall -Wextra
#DEBUG = -g -Wall -Wextra
DEBUG = -g

ifeq ($(TARGET),linux)
	INCLUDE = -I/usr/include
	BIN = mesaflash
	LIBS = -lpci
endif

ifeq ($(TARGET),windows)
	MINGW = c:/MinGW
	INCLUDE = -I$(MINGW)/include
	BIN = mesaflash.exe
	LIBS = -lwsock32 libpci.dll winio32.dll
	DEBUG += -mno-ms-bitfields
endif

CFLAGS = $(INCLUDE) $(OPT) $(DEBUG) $(MATHLIB)

objects = common.o lbp.o bitfile.o hostmot2.o eeprom.o anyio.o eth_boards.o epp_boards.o usb_boards.o pci_boards.o
objects += sserial_module.o eeprom_local.o eeprom_remote.o spi_boards.o spilbp.o main.o

headers = eth_boards.h pci_boards.h epp_boards.h usb_boards.h spi_boards.h anyio.h hostmot2.h lbp16.h common.h eeprom.h
headers += lbp.h eeprom_local.h eeprom_remote.h spilbp.h bitfile.h sserial_module.h hostmot2_def.h

$(BIN) all : $(objects)
	$(CC) -o $(BIN) $(objects) $(MATHLIB) $(LIBS)

main.o : main.c $(headers)
	$(CC) $(CFLAGS) -c main.c

anyio.o : anyio.c $(headers)
	$(CC) $(CFLAGS) -c anyio.c

eth_boards.o : eth_boards.c $(headers)
	$(CC) $(CFLAGS) -c eth_boards.c

pci_boards.o : pci_boards.c $(headers)
	$(CC) $(CFLAGS) -c pci_boards.c

epp_boards.o : epp_boards.c $(headers)
	$(CC) $(CFLAGS) -c epp_boards.c

usb_boards.o : usb_boards.c $(headers) 
	$(CC) $(CFLAGS) -c usb_boards.c

spi_boards.o : spi_boards.c $(headers)
	$(CC) $(CFLAGS) -c spi_boards.c

sserial_module.o : sserial_module.c $(headers)
	$(CC) $(CFLAGS) -c sserial_module.c

eeprom_local.o : eeprom_local.c $(headers)
	$(CC) $(CFLAGS) -c eeprom_local.c

eeprom_remote.o : eeprom_remote.c $(headers)
	$(CC) $(CFLAGS) -c eeprom_remote.c

lbp.o : lbp.c $(headers)
	$(CC) $(CFLAGS) -c lbp.c

spilbp.o : spilbp.c $(headers)
	$(CC) $(CFLAGS) -c spilbp.c

hostmot2.o : hostmot2.c $(headers)
	$(CC) $(CFLAGS) -c hostmot2.c

eeprom.o : eeprom.c $(headers)
	$(CC) $(CFLAGS) -c eeprom.c

bitfile.o : bitfile.c $(headers)
	$(CC) $(CFLAGS) -c bitfile.c

common.o : common.c $(headers)
	$(CC) $(CFLAGS) -c common.c

clean :
	$(RM) $(BIN) *.o

.PHONY: install
install: $(BIN)
       install --mode=0755 --owner root --group root --dir $(DESTDIR)/bin
       install --mode=0755 --owner root --group root $(BIN) $(DESTDIR)/bin
