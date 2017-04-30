CC=gcc
CFLAGS=-Wall -I. -g
VALGRIND_CFLAGS=$(CFLAGS) -O0
LDFLAGS=-lncursesw

tonne: tonne.o fslice.o screen.o editor.o
	$(CC) -o tonne tonne.o fslice.o screen.o editor.o $(CFLAGS) $(LDFLAGS)

tonne-memcheck: tonne.c fslice.c screen.c editor.c
	$(CC) -o tonne-memcheck tonne.c fslice.c screen.c editor.c $(VALGRIND_CFLAGS) $(LDFLAGS)

valgrind: tonne-memcheck
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		--log-file=valgrind-%p.log ./tonne-memcheck Makefile

clean:
	rm -f valgrind-*.log *.o tonne tonne-memcheck
