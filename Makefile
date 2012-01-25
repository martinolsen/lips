CFLAGS = -std=c99 -Wall -Werror -Wextra -g -rdynamic -lm -lao

all: lips test

lips: lips.c audio.c lexer.c lisp.c
	cc $(CFLAGS) -o lips lips.c audio.c lexer.c

test: test.c list.c lexer.c lisp.c
	cc $(CFLAGS) -lcunit -o test test.c list.c lexer.c lisp.c
