all: lips


lips: lips.c audio.c lexer.c
	cc -std=c99 -Wall -Werror -g -lm -lao -o lips lips.c audio.c lexer.c
