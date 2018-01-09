all: client server

client: mrna-client.c
		gcc -std=c99 -pedantic -Wall -D_BSD_SOURCE -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200809L -o mrna-client mrna-client.c

server: mrna-server.c
		gcc -std=c99 -pedantic -Wall -D_BSD_SOURCE -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200809L -o mrna-server mrna-server.c

clean:
		rm -f mrna-client mrna-server
