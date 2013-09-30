
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

objects = main.o eth.o lbp16.o

all : $(objects)
	$(CC) -o $(BIN) $(objects) $(MATHLIB) $(LIBS)

main.o : main.c eth.h
	$(CC) $(CFLAGS) -c main.c

eth.o : eth.c eth.h hostmot2.h
	$(CC) $(CFLAGS) -c eth.c

lbp16.o : lbp16.c lbp16.h
	$(CC) $(CFLAGS) -c lbp16.c

clean :
	$(RM) $(BIN) *.o
