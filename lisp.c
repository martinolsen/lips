#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lisp.h"
#include "lexer.h"
#include "list.h"

#define ERROR(s) fprintf(stderr, "%s+%d: error: %s\n", __FILE__, __LINE__, s);

static object_atom_t *read_atom(lisp_t *, lexer_t *);
static atom_string_t *read_atom_string(lexer_token_t *);
static atom_integer_t *read_atom_integer(lexer_token_t *);
static atom_symbol_t *read_atom_symbol(lexer_token_t *);

static object_list_t *read_list(lisp_t *, lexer_t *);
static object_t *eval_atom(lisp_t *, object_atom_t *);
static object_t *eval_list(lisp_t *, object_list_t *);

static const char *print_atom(lisp_t * lisp, object_atom_t *);
static const char *print_atom_integer(atom_integer_t *);
static const char *print_atom_string(atom_string_t *);
static const char *print_list(lisp_t * lisp, object_list_t *);

lisp_t *lisp_new() {
    lisp_t *lisp = calloc(1, sizeof(lisp_t));

    if(lisp == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    return lisp;
}

void lisp_destroy(lisp_t * lisp) {
    free(lisp);
}

sexpr_t *lisp_read(lisp_t * lisp, const char *s, size_t len) {
    lexer_t *lexer = lexer_init(s, len);

    if(lexer == NULL)
        return NULL;

    sexpr_t *sexpr = calloc(1, sizeof(sexpr_t));

    if(sexpr == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    sexpr->object = (object_t *) read_atom(lisp, lexer);
    if(sexpr->object != NULL)
        return sexpr;

    sexpr->object = (object_t *) read_list(lisp, lexer);
    if(sexpr->object != NULL)
        return sexpr;

    return NULL;
}

object_t *lisp_eval(lisp_t * lisp, sexpr_t * sexpr) {
    if(sexpr->object == NULL)
        return NULL;

    switch (sexpr->object->type) {
    case OBJECT_ATOM:
        return eval_atom(lisp, (object_atom_t *) sexpr->object);
    case OBJECT_LIST:
        return eval_list(lisp, (object_list_t *) sexpr->object);
    case OBJECT_ERROR:
        ERROR("unexpected error");
    }

    return NULL;
}

// Render object to string. Client must free() returned value.
const char *lisp_print(lisp_t * lisp, object_t * object) {
    if(object == NULL)
        return "()";

    switch (object->type) {
    case OBJECT_ATOM:
        return print_atom(lisp, (object_atom_t *) object);
    case OBJECT_LIST:
        return print_list(lisp, (object_list_t *) object);
    case OBJECT_ERROR:
        ERROR("expected atom or list");
    }

    return NULL;
}

static object_atom_t *read_atom(lisp_t * lisp, lexer_t * lexer) {
    lexer_token_t *token = lexer_expect(lexer, TOKEN_ATOM);

    if(token == NULL)
        return NULL;

    object_atom_t *object = calloc(1, sizeof(object_atom_t));

    if(object == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    object->object.type = OBJECT_ATOM;

    object->atom = (atom_t *) read_atom_string(token);
    if(object->atom != NULL) {
        return object;
    }

    object->atom = (atom_t *) read_atom_integer(token);
    if(object->atom != NULL) {
        return object;
    }

    object->atom = (atom_t *) read_atom_symbol(token);
    if(object->atom != NULL) {
        return object;
    }

    free(object);

    ERROR("expected atom of symbol");
    return NULL;
}

static atom_symbol_t *read_atom_symbol(lexer_token_t * token) {
    if(token->type != TOKEN_ATOM)
        return NULL;

    atom_symbol_t *symbol = calloc(1, sizeof(atom_symbol_t));

    if(symbol == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    symbol->atom.type = ATOM_SYMBOL;
    symbol->name = calloc(1, token->len + 1);
    strncpy((char *restrict) symbol->name, token->text, token->len);

    return symbol;
}

static atom_string_t *read_atom_string(lexer_token_t * token) {
    if((token == NULL) || (token->type != TOKEN_ATOM) || (token->len == 0))
        return NULL;

    // String tokens must be surrounded by quotes
    if((token->text[0] != '"') || (token->text[token->len] != '"'))
        return NULL;

    atom_string_t *atom = calloc(1, sizeof(atom_string_t));

    if(atom == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    atom->atom.type = ATOM_STRING;
    atom->len = token->len - 1;
    atom->string = calloc(1, sizeof(atom->len));
    strncpy((char *restrict) atom->string, token->text + 1, token->len + 2);

    return atom;
}

static atom_integer_t *read_atom_integer(lexer_token_t * token) {
    if((token == NULL) || (token->type != TOKEN_ATOM) || (token->len == 0))
        return NULL;

    // Check token is all numbers [0..9]
    for(int i = 0; i < token->len; i++) {
        if(!isdigit(token->text[i]))
            return NULL;
    }

    atom_integer_t *integer = calloc(1, sizeof(atom_symbol_t));

    if(integer == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    integer->atom.type = ATOM_INTEGER;
    integer->number = atoi(token->text);

    return integer;
}

static object_list_t *read_list(lisp_t * lisp, lexer_t * lexer) {
    lexer_token_t *token = lexer_expect(lexer, TOKEN_LIST_START);

    if(token == NULL)
        return NULL;

    object_list_t *object = calloc(1, sizeof(object_list_t));

    if(object == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    object->object.type = OBJECT_LIST;
    object->list = list_new();

    while(NULL == lexer_expect(lexer, TOKEN_LIST_END)) {
        list_node_t *node = list_node_new(NULL);

        node->object = read_atom(lisp, lexer);
        if(node->object != NULL) {
            list_push(object->list, node);
            continue;
        }

        node->object = read_list(lisp, lexer);
        if(node->object != NULL) {
            list_push(object->list, node);
            continue;
        }

        ERROR("expected atom or list");
        free(object);
        return NULL;
    }

    return object;
}

static object_t *eval_atom(lisp_t * lisp, object_atom_t * atom) {
    return (object_t *) atom;
}

static object_t *eval_list(lisp_t * lisp, object_list_t * list) {
    fprintf(stderr, "eval_list\n");

    if(list->list->first == NULL) {
        object_t *object = calloc(1, sizeof(object_t));

        if(object == NULL) {
            perror("calloc");
            exit(EXIT_FAILURE);
        }

        return object;
    }

    object_t *first_object = (object_t *) list->list->first;

    if(first_object->type != OBJECT_ATOM) {
        ERROR("expecting atom as first element of list");
        return NULL;
    }

    if((((object_atom_t *) first_object)->atom)->type != ATOM_SYMBOL) {
        ERROR("expecting symbol atom as first element of list");
        return NULL;
    }

    return NULL;

    //list_t *args = list_new();

    // TODO: Get arguments (rest of list), evaluate on function

    //object_t *val = eval_fun(lisp, fun_symbol, args);

    //list_destroy(args);

    //return val;
}

static const char *print_atom(lisp_t * lisp, object_atom_t * object) {
    switch (object->atom->type) {
    case ATOM_STRING:
        return print_atom_string((atom_string_t *) object->atom);
    case ATOM_SYMBOL:
        return ((atom_symbol_t *) object->atom)->name;
    case ATOM_INTEGER:
        return print_atom_integer((atom_integer_t *) object->atom);
    }

    ERROR("cannot print unknown atom type");
    return NULL;
}

static const char *print_atom_string(atom_string_t * atom) {
    const size_t len = atom->len + 3;
    char *s = calloc(len, sizeof(char));

    if(snprintf(s, len, "\"%s\"", atom->string) == 0) {
        free(s);
        return NULL;
    }

    return s;
}

static const char *print_atom_integer(atom_integer_t * integer) {
    const size_t len = 15;
    char *s = calloc(len + 1, sizeof(char));

    if(s == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    if(snprintf(s, 10, "%d", integer->number) == 0) {
        free(s);
        return NULL;
    }

    return s;
}

static const char *print_list(lisp_t * lisp, object_list_t * object) {
    const size_t len = 255;
    char *s = calloc(len + 1, sizeof(char));

    if((object->list == NULL) || (object->list->first == NULL)) {
        strcpy(s, "()");
        return s;
    }

    size_t idx = 0;
    list_node_t *node = object->list->first;

    do {
        if(idx > len) {
            ERROR("idx > len");
            free(s);
            return NULL;
        }

        const char *n = lisp_print(lisp, (object_t *) node->object);

        int i;

        if(0 ==
           (i = snprintf(s + idx, len - idx, "%s%s", idx ? " " : "(", n))) {
            ERROR("list too long");
            free((char *) n);
            free(s);
            return NULL;
        }

        idx += i;

        free((char *) n);
    } while((node = node->next) != NULL);

    snprintf(s + idx, len - idx, ")");

    return s;
}
