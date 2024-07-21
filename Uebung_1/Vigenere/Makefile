
# @file   Makefile
# @author Vedad Hadzic (e12042758@student.tuwien.ac.at)
# @date   29.10.2023
# @brief Makefile for vigenere.c
 
CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)

OBJECTS = vigenere.o 

.PHONY: all clean
all: vigenere

vigenere: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


vigenere.o: vigenere.c 


clean:
	rm -rf *.o vigenere

release:
	tar -cvzf project-1a.tar.gz Makefile vigenere.c