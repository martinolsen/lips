#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "lexer.h"

lexer_t *lexer_init(const char *str, size_t len) {
    lexer_t *lexer = calloc(1, sizeof(lexer_t));

    lexer->text = str;
    lexer->len = len;
    lexer->pos = 0;

    return lexer;
}

bool accept(lexer_t * lexer, const char *c) {
    if(c == NULL)
        return false;

    if(lexer->pos >= lexer->len)
        return false;

    for(int i = 0; i < strlen(c); i++) {
        if(lexer->text[lexer->pos] == c[i]) {
            lexer->pos++;
            return true;
        }
    }

    return false;
}

void clear_whitespace(lexer_t * lexer) {
    while(accept(lexer, " \t\n"))
        /* NOP */ ;
}

// Like strcspn, but limited by len.
size_t strncspn(const char *s1, size_t len, const char *s2) {
    const char *sc1;

    printf(":%s\n", s1);
    for(sc1 = s1; ((s1 - sc1) < len) && (*sc1 != '\0'); sc1++) {
        printf("%p %p\n", s1, sc1);
        if(strchr(s2, *sc1) != NULL)
            return (sc1 - s1);
    }

    return sc1 - s1;
}

#define D printf("%s+%d: %d/%d\n", __FILE__, __LINE__, (int) lexer->pos, (int)lexer->len);

lexer_token_t *lexer_next(lexer_t * lexer) {
    lexer_token_t *token = calloc(1, sizeof(lexer_token_t));

    if(token == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    clear_whitespace(lexer);

    if(lexer->pos == lexer->len) {
        token->type = TOKEN_EOF;
        return token;
    }

    if(accept(lexer, "(")) {
        token->type = TOKEN_LIST_START;
        return token;
    }
    if(accept(lexer, ")")) {
        token->type = TOKEN_LIST_END;
        return token;
    }

    size_t l = strncspn(lexer->text + lexer->pos, lexer->len, " \t\n()");

    if(l > 0) {
        token->type = TOKEN_ATOM;
        token->text = lexer->text;
        token->len = l;
        lexer->pos += l;
    }
    else {
        token->type = TOKEN_ERROR;
    }

    return token;
}

void lexer_close(lexer_t * lexer) {
    free(lexer);
}
