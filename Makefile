LDFLAGS = -lm -lao
CFLAGS = -std=c99 -Wall -Werror -Wextra -g -rdynamic

all: lips test

%.o: $*.c
	cc $(CFLAGS) -c $@

lips: lips.o audio.o lexer.o lisp.o
	cc $(CFLAGS) $(LDFLAGS) -o $@ $^

test: test.o list.o lexer.o lisp.o
	cc $(CFLAGS) $(LDFLAGS) -lcunit -o $@ $^
	time ./test

clean:
	rm test lips *.o
