all: mydiff

mydiff: main.c
		gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -o mydiff main.c

clean:
		rm -f mydiff
