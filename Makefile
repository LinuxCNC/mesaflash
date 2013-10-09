
CC = gcc
INCLUDE = -I/usr/include
BIN = mesaflash
LIBS = -lpci

RM = rm -f

MATHLIB = -lm
OPT = -O0
#DEBUG = -g -pedantic -Wall -Wextra
#DEBUG = -g -Wall -Wextra
DEBUG = -g -Wall
CFLAGS = $(INCLUDE) $(OPT) $(DEBUG) $(MATHLIB)

objects = common.o lbp16.o bitfile.o hostmot2.o spi_eeprom.o eth_boards.o lpt_boards.o usb_boards.o pci_boards.o spi_access_hm2.o main.o

all : $(objects)
	$(CC) -o $(BIN) $(objects) $(MATHLIB) $(LIBS)

main.o : main.c eth_boards.h pci_boards.h lpt_boards.h usb_boards.h
	$(CC) $(CFLAGS) -c main.c

eth_boards.o : eth_boards.c eth_boards.h hostmot2.h lbp16.h common.h spi_eeprom.h
	$(CC) $(CFLAGS) -c eth_boards.c

pci_boards.o : pci_boards.c pci_boards.h hostmot2.h anyio.h bitfile.h common.h spi_eeprom.h
	$(CC) $(CFLAGS) -c pci_boards.c

lpt_boards.o : lpt_boards.c lpt_boards.h hostmot2.h bitfile.h common.h spi_eeprom.h
	$(CC) $(CFLAGS) -c lpt_boards.c

usb_boards.o : usb_boards.c usb_boards.h hostmot2.h bitfile.h common.h spi_eeprom.h
	$(CC) $(CFLAGS) -c usb_boards.c

spi_access_hm2.o : spi_access_hm2.c spi_access_hm2.h hostmot2.h
	$(CC) $(CFLAGS) -c spi_access_hm2.c

lbp16.o : lbp16.c lbp16.h spi_eeprom.h
	$(CC) $(CFLAGS) -c lbp16.c

hostmot2.o : hostmot2.c hostmot2.h
	$(CC) $(CFLAGS) -c hostmot2.c

spi_eeprom.o : spi_eeprom.c spi_eeprom.h spi_access_hm2.h
	$(CC) $(CFLAGS) -c spi_eeprom.c

bitfile.o : bitfile.c bitfile.h
	$(CC) $(CFLAGS) -c bitfile.c

common.o : common.c common.h
	$(CC) $(CFLAGS) -c common.c

clean :
	$(RM) $(BIN) *.o
