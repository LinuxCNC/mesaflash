
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

objects = main.o eth_boards.o lbp16.o hostmot2.o pci_boards.o lpt_boards.o eeprom.o bitfile.o

all : $(objects)
	$(CC) -o $(BIN) $(objects) $(MATHLIB) $(LIBS)

main.o : main.c eth_boards.h pci_boards.h lpt_boards.h
	$(CC) $(CFLAGS) -c main.c

eth_boards.o : eth_boards.c eth_boards.h hostmot2.h lbp16.h
	$(CC) $(CFLAGS) -c eth_boards.c

pci_boards.o : pci_boards.c pci_boards.h hostmot2.h anyio.h
	$(CC) $(CFLAGS) -c pci_boards.c

lpt_boards.o : lpt_boards.c lpt_boards.h hostmot2.h
	$(CC) $(CFLAGS) -c lpt_boards.c

lbp16.o : lbp16.c lbp16.h
	$(CC) $(CFLAGS) -c lbp16.c

hostmot2.o : hostmot2.c hostmot2.h
	$(CC) $(CFLAGS) -c hostmot2.c

eeprom.o : eeprom.c eeprom.h
	$(CC) $(CFLAGS) -c eeprom.c

bitfile.o : bitfile.c bitfile.h
	$(CC) $(CFLAGS) -c bitfile.c

clean :
	$(RM) $(BIN) *.o
