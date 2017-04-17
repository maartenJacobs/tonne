CC=gcc
CFLAGS=-Wall -I.
VALGRIND_CFLAGS=$(CFLAGS) -O0 -g
LDFLAGS=-lncurses

tonne: tonne.o fslice.o
	$(CC) -o tonne tonne.o fslice.o $(CFLAGS) $(LDFLAGS)

tonne-memcheck: tonne.c
	$(CC) -o tonne-memcheck tonne.c $(VALGRIND_CFLAGS) $(LDFLAGS)

valgrind: tonne-memcheck
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		--log-file=valgrind-%p.log ./tonne-memcheck Makefile

clean:
	rm -f valgrind-*.log tonne.o tonne tonne-memcheck
