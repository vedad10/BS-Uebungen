# @file   Makefile
# @author Vedad Hadzic (e12042758@student.tuwien.ac.at)
# @date   14.01.2024
# @brief Makefile for client.c

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -lrt -lz

OBJECTS = client.o 

.PHONY: all clean release
all: client

client: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

client.o: client.c

clean:
	rm -rf *.o client

release:
	tar -cvzf client.tar.gz Makefile client.c
