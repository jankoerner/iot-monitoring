CC = gcc
CFLAGS = -Wall -pedantic -Wincompatible-pointer-types -Wno-deprecated-declarations -g -O3 -D=CONNECT_SERVER
BDIR = build

all: main.o pla.o
	$(CC) $(CFLAGS) $(BDIR)/main.o $(BDIR)/pla.o -o $(BDIR)/pla -lm

main.o:
	$(CC) $(CFLAGS) -c main.c -o $(BDIR)/main.o

pla.o:
	$(CC) $(CFLAGS) -c pla.c -o $(BDIR)/pla.o

clean:
	rm -rf build
