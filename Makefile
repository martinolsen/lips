CC = gcc
CFLAGS = -std=c99 -Wall -Werror -Wextra -g -rdynamic
LDFLAGS = -lm -lao

.PHONY: run-test clean all

all: lips test

run-test: test
	./test

lips: lips.o audio.o lexer.o lisp.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

test: test.o list.o lexer.o lisp.o
	$(CC) $(CFLAGS) $(LDFLAGS) -lcunit -o $@ $^

.c.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	$(CC) $(CFLAGS) -o $*.d -MM $<

clean:
	rm -f test lips *.o *.d

-include $(wildcard *.d)
