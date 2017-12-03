all: dsort

dsort: dsort.c
		gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -o dsort dsort.c

clean:
		rm -f dsort
