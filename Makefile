
CC = gcc
INCLUDE = -I/usr/include
BIN = mesaflash
LIBS = -lpci

RM = rm -f

MATHLIB = -lm
OPT = -O0
#DEBUG = -g -pedantic -Wall -Wextra
DEBUG = -g -Wall -Wextra
CFLAGS = $(INCLUDE) $(OPT) $(DEBUG) $(MATHLIB)

objects = main.o

all : $(objects)
	$(CC) -o $(BIN) $(objects) $(MATHLIB) $(LIBS)

main.o : main.c
	$(CC) $(CFLAGS) -c main.c

clean :
	$(RM) $(BIN) *.o
