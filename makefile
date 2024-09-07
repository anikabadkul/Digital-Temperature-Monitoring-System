CC=gcc
CFLAGS=-lpthread -I.

test: Lab-5.c
	$(CC) -o test Lab-5.c $(CFLAGS)
.PHONY: clean
clean:
	rm -f test