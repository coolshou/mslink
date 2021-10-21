CC=gcc
CFLAGS=-O2 -s -Wall

OS := $(shell uname)
ifeq ($(OS),Linux)
EXT =
else
EXT =.exe
endif

default: mslink

mslink: mslink.c
	$(CC) $(CFLAGS) $^ -o $@$(EXT)

clean:
	rm -f mslink$(EXT)
