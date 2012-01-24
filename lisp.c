#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lisp.h"
#include "lexer.h"
#include "list.h"

#define ERROR(s) fprintf(stderr, "%s+%d: error: %s\n", __FILE__, __LINE__, s);

static void *ALLOC(size_t);

static object_atom_t *read_atom(lisp_t *, lexer_t *);
static atom_string_t *read_atom_string(lexer_token_t *);
static atom_integer_t *read_atom_integer(lexer_token_t *);
static atom_symbol_t *read_atom_symbol(lexer_token_t *);

static object_list_t *read_list(lisp_t *, lexer_t *);

static object_t *eval_atom(lisp_t *, object_atom_t *);
static object_t *eval_list(lisp_t *, object_list_t *);

static const char *print_atom(lisp_t *, object_atom_t *);
static const char *print_atom_integer(atom_integer_t *);
static const char *print_atom_string(atom_string_t *);
static const char *print_list(lisp_t *, object_list_t *);
static const char *print_object(lisp_t *, object_t *);
static const char *print_cons(lisp_t *, object_cons_t *);

static object_list_t *cons(object_t *, object_list_t *);

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

    ERROR("lisp_read: unexpected error");
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
    case OBJECT_CONS:
        ERROR("will not evaluate CONS object");
    case OBJECT_ERROR:
        ERROR("lisp_eval: unexpected error");
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
    case OBJECT_CONS:
        ERROR("CONS' are not printable");
    case OBJECT_ERROR:
        ERROR("lisp_print: unexpected error");
    }

    return NULL;
}

static void *ALLOC(const size_t sz) {
    void *o = calloc(1, sz);

    if(o == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    return o;
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

    object_list_t *list = ALLOC(sizeof(object_list_t));
    object_cons_t *cons = ALLOC(sizeof(object_cons_t));

    list->object.type = OBJECT_LIST;
    list->cons = cons;
    cons->object.type = OBJECT_CONS;

    while(NULL == lexer_expect(lexer, TOKEN_LIST_END)) {
        if(cons->car == NULL)
            cons->car = (object_t *) read_atom(lisp, lexer);
        if(cons->car == NULL)
            cons->car = (object_t *) read_list(lisp, lexer);

        if(cons->car == NULL) {
            ERROR("read_list: expected atom or list");
            return NULL;
        }

        cons->cdr = ALLOC(sizeof(object_cons_t));
        cons->cdr->type = OBJECT_CONS;
        cons = (object_cons_t *) cons->cdr;
    }

    return list;
}

static object_t *eval_atom(lisp_t * lisp, object_atom_t * atom) {
    return (object_t *) atom;
}

static object_t *eval_list(lisp_t * lisp, object_list_t * list) {
    if((list->cons == NULL) || (list->cons->car == NULL))
        return (object_t *) list;

    object_t *car = list->cons->car;

    if(car->type != OBJECT_ATOM) {
        ERROR("expecting atom as first element of list");
        return NULL;
    }

    if((((object_atom_t *) car)->atom)->type != ATOM_SYMBOL) {
        ERROR("expecting symbol atom as first element of list");
        return NULL;
    }

    atom_symbol_t *fun_symbol =
        (atom_symbol_t *) ((object_atom_t *) car)->atom;

    if(strncmp(fun_symbol->name, "CONS", 5) == 0) {
        object_cons_t *cdr = (object_cons_t *) list->cons->cdr;

        if(cdr == NULL) {
            ERROR("cdr is NULL");
            return NULL;
        }

        if(cdr->object.type != OBJECT_CONS) {
            fprintf(stderr, "%d\n", cdr->object.type);
            ERROR("cdr is not a CONS object");
            return NULL;
        }

        object_t *cdar = cdr->car;

        if(cdar->type != OBJECT_ATOM) {
            ERROR("CDAR is not atom object");
            return NULL;
        }

        object_cons_t *cddr = (object_cons_t *) cdr->cdr;

        if(cddr->object.type != OBJECT_CONS) {
            ERROR("CDDR is not CONS object");
            return NULL;
        }

        object_list_t *cddar = (object_list_t *) cddr->car;

        if((cddar != NULL) && (cddar->object.type != OBJECT_LIST)) {
            fprintf(stderr, "CDDAR: %s (%d)\n",
                    print_object(lisp, (object_t *) cddar),
                    cddar->object.type);

            ERROR("CDDAR is not a LIST");
            return NULL;
        }

        object_t *head = cdar;
        object_list_t *tail = cddar;

        if((tail != NULL) && (tail->object.type != OBJECT_LIST)) {
            fprintf(stderr, "tail is %d\n", tail->object.type);

            ERROR("tail is not a LIST");
            return NULL;
        }

        return (object_t *) cons(head,
                                 (object_list_t *) eval_list(lisp, tail));
    }

    fprintf(stderr, "function name: %s\n", fun_symbol->name);
    ERROR("eval_list: unknown function name");
    return NULL;
}

// apply a to list ==> (a . b)
static object_list_t *cons(object_t * x, object_list_t * list) {
    if(x == NULL)
        return list;

    if(list == NULL) {
        fprintf(stderr, "CONS - new list\n");

        list = ALLOC(sizeof(object_list_t));
        list->object.type = OBJECT_LIST;
    }

    if((list->cons != NULL) && (list->cons->object.type != OBJECT_CONS)) {
        ERROR("MEIN LEBEN!");
        return NULL;
    }

    object_cons_t *tail = list->cons;

    list->cons = ALLOC(sizeof(object_cons_t));
    list->cons->object.type = OBJECT_CONS;
    list->cons->car = x;
    list->cons->cdr = (object_t *) tail;

    return list;
}

static const char *print_object(lisp_t * lisp, object_t * o) {
    if(o == NULL)
        return "";

    switch (o->type) {
    case OBJECT_ERROR:
        return "ERR";
    case OBJECT_ATOM:
        return print_atom(lisp, (object_atom_t *) o);
    case OBJECT_CONS:
        return print_cons(lisp, (object_cons_t *) o);
    case OBJECT_LIST:
        return print_list(lisp, (object_list_t *) o);
    }

    ERROR("print_object: unknwon object");
    return "N/A";
}

static const char *print_cons(lisp_t * lisp, object_cons_t * object) {
    const size_t len = 1024;
    char *s = ALLOC(len * sizeof(char));

    object_t *car = (object_t *) object->car;
    object_t *cdr = (object_t *) object->cdr;

    snprintf(s, len, "%s %s\n", print_object(lisp, car),
             print_object(lisp, cdr));

    return s;
}

static const char *print_atom(lisp_t * lisp, object_atom_t * object) {
    switch (object->atom->type) {
    case ATOM_STRING:
        return print_atom_string((atom_string_t *) object->atom);
    case ATOM_SYMBOL:
        return ((atom_symbol_t *) object->atom)->name;
    case ATOM_INTEGER:
        return print_atom_integer((atom_integer_t *) object->atom);
    case ATOM_ERROR:
        ERROR("print_atom: unknown error");
    }

    fprintf(stderr, "%d\n", object->atom->type);

    ERROR("print_atom: cannot print unknown atom type");
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

static const char *print_list(lisp_t * lisp, object_list_t * list) {
    const size_t len = 255;
    char *s = ALLOC(len * sizeof(char) + 1);

    if((list == NULL) || (list->cons == NULL) || (list->cons->car == NULL)) {
        strcpy(s, "NIL");
        return s;
    }

    object_cons_t *cons = list->cons;

    size_t si = 0;

    while(cons->car != NULL) {
        if(si > len) {
            ERROR("si > len");
            free(s);
            return NULL;
        }

        const char *n = lisp_print(lisp, cons->car);
        int i = snprintf(s + si, len - si, "%s%s", si ? " " : "(", n);

        if(i == 0) {
            ERROR("list too long");
            free(s);
            return NULL;
        }

        si += i;

        cons = (object_cons_t *) cons->cdr;

        if(cons->object.type != OBJECT_CONS) {
            ERROR("error in list");
            return NULL;
        }
    };

    snprintf(s + si, len - si, ")");

    return s;
}
