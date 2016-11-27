all: hashsum

client: client.c
	gcc -std=c99 -pedantic -Wall -g -DENDEBUG -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -o hashsum hashsum.c

clean:
	rm -f client server
