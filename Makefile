CC=gcc
CFLAGS=-Wall -I.
VALGRIND_CFLAGS=$(CFLAGS) -O0
LDFLAGS=-lncurses

tonne: tonne.o
	$(CC) -o tonne tonne.o $(CFLAGS) $(LDFLAGS)

tonne-memcheck: tonne.c
	$(CC) -o tonne-memcheck tonne.c $(VALGRIND_CFLAGS) $(LDFLAGS)

valgrind: tonne-memcheck
	valgrind --leak-check=full --track-origins=yes ./tonne-memcheck tonne.c
