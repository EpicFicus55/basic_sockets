CC = gcc
CFLAGS = -Wall -O2

.PHONY: all clean

all: client server

client: client.c
	${CC} ${CFLAGS} client.c -o client

server: server.c
	${CC} ${CFLAGS} server.c -o server
	
	mkdir -p remote

clean:
	rm client server
