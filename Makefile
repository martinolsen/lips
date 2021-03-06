CC = gcc
CFLAGS = -std=c99 -Wall -Werror -Wextra -g -rdynamic
LDFLAGS = -lm -lreadline

.PHONY: run-test clean all tags

all: lips test tags

tags: $(wildcard *.c) $(wildcard *.h)
	ctags *.c

run-test: all
	./test $(TESTFLAGS)

OBJS = logger.o object.o stream.o builtin.o lisp_print.o lisp_eval.o lisp.o \
       lisp_read.o

lips: lips.o repl.o $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

test: test.o list.o $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -lcunit -o $@ $^

.c.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	$(CC) $(CFLAGS) -o $*.d -MM $<

clean:
	rm -f test lips *.o *.d

-include $(wildcard *.d)
