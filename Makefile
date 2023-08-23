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
LIBNAME = libanyio
LIBANYIO = $(LIBNAME).a

CC = gcc
RM = rm -f
AR = ar
RANLIB = ranlib

OWNERSHIP ?= --owner root --group root

# default CFLAGS
CFLAGS ?= -O0 -g -Wall -Wextra -Werror

# mesaflash needs at least C99 to compile.
#
# The oldest distros we support are Debian Wheezy (EOL 2018-05-31)
# and Ubuntu Precise (EOL 2017-04-28):
#
#     Debian Wheezy has gcc 4.7.2, which defaults to C90 but supports C11.
#
#     Ubuntu Precise has gcc 4.6.3, which defaults to C90 but supports
#     C99 (but does not support C11).
#
# So we explicitly select the newest ancient C standard that we have to
# support here.
CFLAGS += -std=c99

ifeq ($(TARGET),linux)
    $(shell which pkg-config > /dev/null)
    ifeq ($(.SHELLSTATUS), 1)
        $(error "can't find pkg-config")
    endif

    $(shell pkg-config --exists libpci > /dev/null)
    ifeq ($(.SHELLSTATUS), 1)
        $(error "pkg-config can't find libpci")
    endif

    $(shell pkg-config --exists libmd > /dev/null)
    ifeq ($(.SHELLSTATUS), 1)
        $(error "pkg-config can't find libmd")
    endif

    LIBPCI_CFLAGS := $(shell pkg-config --cflags libpci)
    LIBPCI_LDFLAGS := $(shell pkg-config --libs libpci)
    LIBMD_CFLAGS := $(shell pkg-config --cflags libmd)
    LIBMD_LDFLAGS := $(shell pkg-config --libs libmd)
    BIN = mesaflash
    LDFLAGS = -lm $(LIBPCI_LDFLAGS) $(LIBMD_LDFLAGS)
    CFLAGS += -D_GNU_SOURCE $(LIBPCI_CFLAGS) $(LIBMD_CFLAGS) -D_FILE_OFFSET_BITS=64

    UNAME_M := $(shell uname -m)

    #
    # A bunch of platforms lack `sys/io.h`, which means mesaflash builds
    # without support for EPP or PCI cards.
    #

    ifeq ($(UNAME_M),aarch64)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(patsubst arm%,arm,$(UNAME_M)),arm)
        ifeq ($(wildcard /usr/include/arm-linux-gnueabihf/asm/io.h),)
            MESAFLASH_IO ?= 0
        endif
    endif

    ifeq ($(UNAME_M),loongarch64)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(UNAME_M),parisc)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(UNAME_M),m68k)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(patsubst mips%,mips,$(UNAME_M)),mips)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(patsubst ppc%,ppc,$(UNAME_M)),ppc)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(UNAME_M),riscv64)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(patsubst s390%,s390,$(UNAME_M)),s390)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(UNAME_M),sh)
        MESAFLASH_IO ?= 0
    endif

    ifeq ($(UNAME_M),sparc64)
        MESAFLASH_IO ?= 0
    endif

endif

ifeq ($(TARGET),windows)
    MINGW = c:/MinGW
    CFLAGS += -I$(MINGW)/include
    BIN = mesaflash.exe
    LDFLAGS = -lwsock32 -lws2_32 libpci.dll winio32.dll
    CFLAGS += -mno-ms-bitfields
endif

MESAFLASH_IO ?= 1
CFLAGS += -DMESAFLASH_IO=$(MESAFLASH_IO)

objects = common.o lbp.o lbp16.o bitfile.o hostmot2.o eeprom.o anyio.o eth_boards.o epp_boards.o usb_boards.o pci_boards.o
objects += sserial_module.o encoder_module.o eeprom_local.o eeprom_remote.o spi_boards.o serial_boards.o

headers = eth_boards.h pci_boards.h epp_boards.h usb_boards.h spi_boards.h serial_boards.h anyio.h hostmot2.h lbp16.h types.h
headers +=  common.h eeprom.h lbp.h eeprom_local.h eeprom_remote.h bitfile.h sserial_module.h hostmot2_def.h boards.h
headers +=  encoder_module.h

all: $(LIBANYIO) $(BIN) pci_encoder_read pci_analog_write

$(LIBANYIO) : $(objects)
	$(RM) $(LIBANYIO) $(BIN)
	$(AR) rcs $(LIBANYIO) $(objects)
	$(RANLIB) $(LIBANYIO)

mesaflash.o : mesaflash.c $(headers)
	$(CC) $(CFLAGS) -c mesaflash.c

$(BIN): mesaflash.o anyio.h $(LIBANYIO)
	$(CC) $(CFLAGS) -o $(BIN) mesaflash.o $(LIBANYIO) $(LDFLAGS)

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

serial_boards.o : serial_boards.c $(headers)
	$(CC) $(CFLAGS) -c serial_boards.c

sserial_module.o : sserial_module.c $(headers)
	$(CC) $(CFLAGS) -c sserial_module.c

encoder_module.o : encoder_module.c $(headers)
	$(CC) $(CFLAGS) -c encoder_module.c

eeprom_local.o : eeprom_local.c $(headers)
	$(CC) $(CFLAGS) -c eeprom_local.c

eeprom_remote.o : eeprom_remote.c $(headers)
	$(CC) $(CFLAGS) -c eeprom_remote.c

lbp.o : lbp.c $(headers)
	$(CC) $(CFLAGS) -c lbp.c

lbp16.o : lbp16.c $(headers)
	$(CC) $(CFLAGS) -c lbp16.c

hostmot2.o : hostmot2.c $(headers)
	$(CC) $(CFLAGS) -c hostmot2.c

eeprom.o : eeprom.c $(headers)
	$(CC) $(CFLAGS) -c eeprom.c

bitfile.o : bitfile.c $(headers)
	$(CC) $(CFLAGS) -c bitfile.c

common.o : common.c $(headers)
	$(CC) $(CFLAGS) -c common.c

pci_encoder_read.o : examples/pci_encoder_read.c $(LIBANYIO) $(headers)
	$(CC) $(CFLAGS) -c examples/pci_encoder_read.c

pci_encoder_read: pci_encoder_read.o anyio.h encoder_module.h
	$(CC) $(CFLAGS) -o pci_encoder_read pci_encoder_read.o $(LIBANYIO) $(LDFLAGS)

pci_analog_write.o : examples/pci_analog_write.c $(LIBANYIO) $(headers)
	$(CC) $(CFLAGS) -c examples/pci_analog_write.c

pci_analog_write: pci_analog_write.o anyio.h sserial_module.h
	$(CC) $(CFLAGS) -o pci_analog_write pci_analog_write.o $(LIBANYIO) $(LDFLAGS)

clean :
	$(RM) *.o $(LIBANYIO) $(BIN) pci_encoder_read pci_analog_write

.PHONY: install
install: $(BIN)
	install -p -D --mode=0755 $(OWNERSHIP) $(BIN) $(DESTDIR)/bin/$(BIN)
	install -p -D --mode=0644 $(OWNERSHIP) mesaflash.1 $(DESTDIR)/share/man/man1/mesaflash.1
