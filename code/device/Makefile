CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -Wincompatible-pointer-types -Wno-deprecated-declarations

all: 
	mkdir -p build
	$(CC) $(CFLAGS) -o build/main main.c

clean:
	rm -rf build
