#ifndef __LEXER_H
#define __LEXER_H

#include <stdlib.h>

#define LOOKAHEAD_MAX 2

typedef enum {
    TOKEN_ERROR,
    TOKEN_ATOM,
    TOKEN_LIST_START,
    TOKEN_LIST_END,
    TOKEN_EOF,
} token_type_t;

typedef struct {
    token_type_t type;
    const char *text;
    size_t len;
} lexer_token_t;

typedef struct lexer_t {
    const char *text;
    size_t len;
    size_t pos;

    lexer_token_t *lookahead[LOOKAHEAD_MAX];
    int lookahead_idx;
} lexer_t;

lexer_t *lexer_init(const char *, size_t);
lexer_token_t *lexer_next(lexer_t *);
lexer_token_t *lexer_expect(lexer_t *, token_type_t);
void lexer_close(lexer_t *);

#endif
