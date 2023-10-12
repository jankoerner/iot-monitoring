CC = gcc
CFLAGS = -Wall -pedantic -Wincompatible-pointer-types -Wno-deprecated-declarations -g
BDIR = build

all: main.o adaptive.o
	$(CC) $(CFLAGS) $(BDIR)/main.o $(BDIR)/adaptive.o -o $(BDIR)/main

main.o:
	$(CC) $(CFLAGS) -c main.c -o $(BDIR)/main.o

adaptive.o:
	$(CC) $(CFLAGS) -c adaptive_sampling.c -o $(BDIR)/adaptive.o

clean:
	rm -rf build
