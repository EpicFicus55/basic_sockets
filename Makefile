CC = gcc
CFLAGS = -std=c99 -Wall -Wpedantic -O2
DEFINES = -D_POSIX_C_SOURCE=200112L

.PHONY: all clean

all: client server

client: client.c
	${CC} ${CFLAGS} ${DEFINES}client.c -o client

server: server.c
	${CC} ${CFLAGS} ${DEFINES} server.c -o server
	
	mkdir -p remote

clean:
	rm client server
	rm -fr ./remote/*
