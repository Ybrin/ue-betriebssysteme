all: client server

client: client.c
		gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -o client client.c

server: server.c
		gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -o server server.c

clean:
		rm -f client server
