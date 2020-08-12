# Makefile for Linux etc.

.PHONY: all clean
all: bladegps

SHELL=/bin/bash
CC=gcc
CFLAGS+=-O3 -Wall -I../bladeRF/host/libraries/libbladeRF/include
LDFLAGS=-lm -lpthread -L../bladeRF/host/build/output -lbladeRF

bladegps: bladegps.o gpssim.o
	${CC} $^ ${LDFLAGS} -o $@

clean:
	rm -f *.o bladegps
