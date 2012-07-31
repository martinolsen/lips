CC = gcc
CFLAGS = -std=c99 -Wall -Werror -Wextra -g -rdynamic
LDFLAGS = 

OBJS = vm.o

.PHONY: test

all: vm

tags: $(wildcard *.c) $(wildcard *.h)
	ctags *.c

test: test_vm.o $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -lcunit -o $@ $^

clean:
	rm -f *.o *.d

.c.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
	$(CC) $(CFLAGS) -o $*.d -MM $<

-include $(wildcard *.d)
