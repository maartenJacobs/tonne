CC=gcc
CFLAGS=-I.

tonne: tonne.o
	$(CC) -o tonne tonne.o -I. -lncurses -Wall