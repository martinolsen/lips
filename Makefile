CC = gcc
CFLAGS = -std=c99 -Wall -Werror -Wextra -g -rdynamic
LDFLAGS = -lm -lao

.PHONY: run-test clean all tags

all: lips test tags

tags: $(wildcard *.c) $(wildcard *.h)
	ctags *.c

run-test: all
	./test

lips: lips.o audio.o lisp_print.o lisp_eval.o lexer.o lisp.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

test: test.o list.o lisp_print.o lisp_eval.o lexer.o lisp.o
	$(CC) $(CFLAGS) $(LDFLAGS) -lcunit -o $@ $^

.c.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	$(CC) $(CFLAGS) -o $*.d -MM $<

clean:
	rm -f test lips *.o *.d

-include $(wildcard *.d)
