CC = gcc
CFLAGS = -O2 -Wall -Wextra

all: bench run

bench: bench.c rebus.c rebus.h
	$(CC) $(CFLAGS) -o $@ bench.c rebus.c

run: run.c rebus.c rebus.h
	$(CC) $(CFLAGS) -o $@ run.c rebus.c

clean:
	rm -f bench run bench.exe run.exe
