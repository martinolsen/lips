#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "lexer.h"

typedef struct {
} sexpr_t;

typedef struct {
} list_t;

typedef struct {
} atom_t;

// Read single atom
static atom_t *read_atom(lexer_t * lexer) {
    atom_t *atom = calloc(1, sizeof(atom_t));

    if(atom == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    return atom;
}

// Read list - from "(" to ")" - return list_t*
static list_t *read_list(lexer_t * lexer) {
    list_t *list = calloc(1, sizeof(list_t));

    if(list == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    while((token = lexer_next(lexer))) {

    }

    return list;
}

static sexpr_t *sexpr_read(const char *s) {
    lexer_t *lexer = lexer_init(s, strlen(s));
    lexer_token_t *token;

    while((token = lexer_peek(lexer))) {
        printf("%d %d\n", (int) lexer->pos, token->type);

        switch (token->type) {
        case TOKEN_LIST_START:
            read_list(lexer);
            // TODO: push list onto sexpr
            break;
        case TOKEN_ATOM:
            read_atom(lexer);
            // TODO: push atom onto sexpr
            printf("atom\n");
            break;
        case TOKEN_LIST_END:
        case TOKEN_ERROR:
            printf("error\n");
            break;
        case TOKEN_EOF:
            printf("eof\n");
            return NULL;
        }
    }

    return NULL;
}

static void beep() {
    printf("beep!\n");
    audio_t *audio = audio_init();

    audio_beep(audio);
    audio_close(audio);
}

int main(int argc, char **argv) {
    fprintf(stderr, "       LISP  Interactive  Performance  System\n");

    if(sexpr_read("(+ 1 2)"))
        beep();

    exit(EXIT_SUCCESS);
}
