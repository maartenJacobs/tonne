CC=gcc
CFLAGS=-Wall -I.
VALGRIND_CFLAGS=$(CFLAGS) -O0 -g
LDFLAGS=-lncurses

tonne: tonne.o
	$(CC) -o tonne tonne.o $(CFLAGS) $(LDFLAGS)

tonne-memcheck: tonne.c
	$(CC) -o tonne-memcheck tonne.c $(VALGRIND_CFLAGS) $(LDFLAGS)

valgrind: tonne-memcheck
	valgrind --leak-check=full --track-origins=yes --log-file=valgrind-%p.log ./tonne-memcheck tonne.c

clean:
	rm -f valgrind-*.log tonne.o tonne tonne-memcheck
