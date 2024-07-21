# @file   Makefile
# @author Vedad Hadzic (e12042758@student.tuwien.ac.at)
# @date   15.01.2024
# @brief Makefile for server.c

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -lrt -lz

OBJECTS = server.o 

.PHONY: all clean release
all: server

server: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

server.o: server.c

clean:
	rm -rf *.o server

release:
	tar -cvzf server.tar.gz Makefile server.c