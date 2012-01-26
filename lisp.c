#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#include "lisp.h"
#include "lexer.h"
#include "list.h"

#define DEBUG(f, args...) do { \
    debug(__FILE__, __LINE__, f, ##args); } while(0);

#define ERROR(f, args...) do { \
    error(__FILE__, __LINE__, f, ##args); } while(0);

#define PANIC(f, args...) do { \
    panic(__FILE__, __LINE__, f, ##args); } while(0);

#define BACKTRACE_MAX 100

#define BACKTRACE do { \
    void *buffer[BACKTRACE_MAX]; \
    int j, nptrs = backtrace(buffer, BACKTRACE_MAX); \
    printf("backtrace() returned %d addresses\n", nptrs); \
    char **strings = backtrace_symbols(buffer, nptrs); \
    if(strings == NULL) { \
        perror("backtrace_symbols"); exit(EXIT_FAILURE); } \
    for (j = 0; j < nptrs; j++) printf("%s\n", strings[j]); \
    free(strings); } while(0);

//static int debug(const char *file, const int line, const char *fmt, ...);
static int error(const char *file, const int line, const char *fmt, ...);
static void panic(const char *file, const int line, const char *fmt, ...);

static void *ALLOC(size_t);

static object_t *object_new(object_type_t type);
static object_atom_t *object_atom_new(atom_t *);
static object_cons_t *object_cons_new(object_t *, object_t *);
static object_function_t *object_function_new(void);

static object_atom_t *symbol(const char *);

static object_t *read_object(lexer_t *);
static object_atom_t *read_atom(lexer_t *);
static atom_string_t *read_atom_string(lexer_token_t *);
static atom_integer_t *read_atom_integer(lexer_token_t *);
static atom_symbol_t *read_atom_symbol(lexer_token_t *);

static object_cons_t *read_list(lexer_t *);

static object_t *eval_object(object_t *);
static object_t *eval_atom(object_atom_t *);
static object_t *eval_list(object_cons_t *);

static const char *print_atom(object_atom_t *);
static const char *print_atom_integer(atom_integer_t *);
static const char *print_atom_string(atom_string_t *);
static const char *print_cons(object_cons_t *);
static const char *print_object(object_t *);
static const char *print_cons(object_cons_t *);

static object_cons_t *cons(object_t *, object_t *);
static object_t *car(object_cons_t *);
static object_t *cdr(object_cons_t *);
static object_t *atom(object_t *);
static object_t *eq(object_t *, object_t *);
static object_t *first(object_cons_t *);
static object_t *second(object_cons_t *);

//static object_t *third(object_cons_t *);
static int list_length(object_cons_t *);

sexpr_t *lisp_read(const char *s, size_t len) {
    lexer_t *lexer = lexer_init(s, len);

    if(lexer == NULL)
        return NULL;

    sexpr_t *sexpr = calloc(1, sizeof(sexpr_t));

    if(sexpr == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    sexpr->object = read_object(lexer);

    return sexpr;
}

object_t *lisp_eval(sexpr_t * sexpr) {
    return eval_object(sexpr->object);
}

static object_t *eval_object(object_t * object) {
    if(object == NULL)
        return NULL;

    switch (object->type) {
    case OBJECT_ATOM:
        return eval_atom((object_atom_t *) object);
    case OBJECT_CONS:
        return eval_list((object_cons_t *) object);
    case OBJECT_FUNCTION:
        PANIC("Trying to evaluate function!");
    case OBJECT_ERROR:
        ERROR("eval_object: unexpected error");
    }

    ERROR("eval_object: unhandled object %s", print_object(object));
    exit(EXIT_FAILURE);
}

// Render object to string. Client must free() returned value.
const char *lisp_print(object_t * object) {
    return print_object(object);
}

static object_t *read_object(lexer_t * lexer) {
    object_t *object;

    object = (object_t *) read_atom(lexer);
    if(object != NULL)
        return object;

    return (object_t *) read_list(lexer);
}

static object_atom_t *read_atom(lexer_t * lexer) {
    lexer_token_t *token = lexer_expect(lexer, TOKEN_ATOM);

    if(token == NULL)
        return NULL;

    object_atom_t *object = object_atom_new(NULL);

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

    ERROR("expected atom of string, integer or symbol");
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
    for(size_t i = 0; i < token->len; i++) {
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

static object_cons_t *read_list(lexer_t * lexer) {
    lexer_token_t *token = lexer_expect(lexer, TOKEN_LIST_START);

    if(token->type != TOKEN_LIST_START)
        PANIC("read_list - no TOKEN_LIST_START!");

    if((token == NULL) || (lexer_expect(lexer, TOKEN_LIST_END)))
        return NULL;

    object_cons_t *list = object_cons_new(NULL, NULL);
    object_cons_t *pair = list;

    while(NULL == lexer_expect(lexer, TOKEN_LIST_END)) {
        pair->car = read_object(lexer);
        pair->cdr = (object_t *) object_cons_new(NULL, NULL);

        pair = (object_cons_t *) pair->cdr;
    }

    return list;
}

static object_t *eval_atom(object_atom_t * atom) {
    return (object_t *) atom;
}

static object_t *C_atom(object_cons_t * args) {
    return atom(eval_object(first(args)));
}

static object_t *C_quote(object_cons_t * args) {
    return first(args);
}

static object_t *C_eq(object_cons_t * args) {
    return eq(eval_object(first(args)), eval_object(second(args)));
}

static object_t *C_cons(object_cons_t * args) {
    return (object_t *) cons(eval_object(first(args)),
                             eval_object(second(args)));
}

static object_t *C_car(object_cons_t * args) {
    return car((object_cons_t *) eval_object(first(args)));
}

static object_t *C_cdr(object_cons_t * args) {
    return cdr((object_cons_t *) eval_object(first(args)));
}

#define ADD_FUN(s, name, fun, argc) do { \
    object_function_t *f = object_function_new(); \
    f->fptr = fun; f->args = argc; \
    s = object_cons_new((object_t *) f, (object_t *) s); \
    s = object_cons_new((object_t *) symbol(name), (object_t *) s); } while(0);

static object_cons_t *make_function_symbols() {
    object_cons_t *symbols = NULL;

    ADD_FUN(symbols, "CONS", C_cons, 2);
    ADD_FUN(symbols, "CAR", C_car, 1);
    ADD_FUN(symbols, "CDR", C_cdr, 1);
    ADD_FUN(symbols, "EQ", C_eq, 2);
    ADD_FUN(symbols, "QUOTE", C_quote, 1);
    ADD_FUN(symbols, "ATOM", C_atom, 1);

    return symbols;
}

static object_function_t *symbol_function(object_t * sym,
                                          object_cons_t * symbols) {

    while(symbols != NULL) {
        if(symbols->object.type != OBJECT_CONS)
            PANIC("invalid symbols LIST");

        object_cons_t *val = (object_cons_t *) cdr(symbols);

        if(eq(car(symbols), (object_t *) sym))
            return (object_function_t *) car(val);

        symbols = (object_cons_t *) cdr(val);
    }

    return NULL;
}

static object_t *eval_list(object_cons_t * list) {
    if(list == NULL)
        return NULL;

    if(car(list) == NULL)
        PANIC("eval_list: function is NULL");

    object_t *prefix = first(list);

    if(!atom(prefix)) {
        ERROR("expecting atom as prefix");
        return NULL;
    }

    if((((object_atom_t *) prefix)->atom)->type != ATOM_SYMBOL) {
        ERROR("expecting symbol atom as first element of list");
        return NULL;
    }

    atom_symbol_t *fun_symbol =
        (atom_symbol_t *) ((object_atom_t *) prefix)->atom;

    object_function_t *fun = symbol_function(prefix, make_function_symbols());

    if(fun == NULL)
        PANIC("eval_list: unknown function name %s", fun_symbol->name);

    if(fun->args != (list_length(list) - 1)) {
        PANIC("function %s takes %d arguments", print_object(prefix),
              fun->args);
    }

    object_t *r = (*fun->fptr) ((object_cons_t *) cdr(list));

    return r;
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// BUILT-IN ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// TODO TODO TODO - rename function, use ENVIRONMENTal lookup
static object_atom_t *symbol(const char *t) {
    object_atom_t *a = object_atom_new(ALLOC(sizeof(atom_symbol_t)));

    a->atom->type = ATOM_SYMBOL;
    ((atom_symbol_t *) a->atom)->name = t;

    return a;
}

// Return true if OBJECT is anything other than a CONS
static object_t *atom(object_t * object) {
    if((object == NULL) || (object->type != OBJECT_CONS))
        return (object_t *) symbol("T");

    return NULL;
}

// apply a to list ==> (a . b)   -- b MUST be a list
static object_cons_t *cons(object_t * a, object_t * b) {
    object_cons_t *cons = object_cons_new(a, b);

    return cons;
}

static object_t *car(object_cons_t * cons) {
    if(cons == NULL)
        PANIC("car(NULL)");

    if(cons->object.type != OBJECT_CONS) {
        ERROR("car: object is not a list: %s (%d)\n",
              print_object((object_t *) cons), cons->object.type);
        exit(EXIT_FAILURE);
    }

    return cons->car;
}

static object_t *cdr(object_cons_t * cons) {
    if((cons == NULL) || (cons->object.type != OBJECT_CONS)) {
        ERROR("cdr: object is not a list: %s\n",
              print_object((object_t *) cons));

        return NULL;
    }

    return cons->cdr;
}

static object_t *eq(object_t * a, object_t * b) {
    if(a == b)
        return (object_t *) symbol("T");

    if((a == NULL) || (b == NULL))
        return NULL;

    if(a->type != b->type)
        return NULL;

    if(a->type == OBJECT_ERROR) {
        PANIC("eq(%s, %s) - type is OBJECT_ERROR", print_object(a),
              print_object(b));
    }

    if(a->type == OBJECT_CONS)
        return NULL;

    if(a->type != OBJECT_ATOM) {
        PANIC("eq(%s, %s) - unknown object type", print_object(a),
              print_object(b));
    }

    if((((object_atom_t *) a)->atom == NULL)
       || (((object_atom_t *) a)->atom == NULL)) {
        PANIC("eq(%s, %s) - invalid object atom", print_object(a),
              print_object(b));
    }

    atom_t *a_atom = ((object_atom_t *) a)->atom;
    atom_t *b_atom = ((object_atom_t *) b)->atom;

    if(a_atom->type != b_atom->type)
        return NULL;

    if(a_atom->type == ATOM_ERROR) {
        PANIC("eq(%s, %s) - type is ATOM_ERROR", print_object(a),
              print_object(b));
    }

    if(a_atom->type == ATOM_SYMBOL) {
        if(0 ==
           strcmp(((atom_symbol_t *) a_atom)->name,
                  ((atom_symbol_t *) b_atom)->name)) {

            return (object_t *) symbol("T");
        }

        return NULL;
    }

    if(a_atom->type == ATOM_STRING) {
        if(0 ==
           strncmp(((atom_string_t *) a_atom)->string,
                   ((atom_string_t *) b_atom)->string,
                   ((atom_string_t *) b_atom)->len)) {

            return (object_t *) symbol("T");
        }

        return NULL;
    }

    if(a_atom->type == ATOM_INTEGER) {
        if(((atom_integer_t *) a_atom)->number ==
           ((atom_integer_t *) b_atom)->number) {

            return (object_t *) symbol("T");
        }

        return NULL;
    }

    PANIC("?");
    return NULL;
}

static object_t *first(object_cons_t * list) {
    if((list == NULL) || (list->object.type != OBJECT_CONS))
        return NULL;

    return car(list);
}

static object_t *second(object_cons_t * list) {
    if((list == NULL) || (list_length(list) < 2))
        return NULL;

    return car((object_cons_t *) cdr(list));
}

#if 0
static object_t *third(object_cons_t * list) {
    if((list == NULL) || (list_length(list) < 3))
        return NULL;

    return car((object_cons_t *) cdr((object_cons_t *) cdr(list)));
}
#endif

static int list_length(object_cons_t * list) {
    if((list == NULL) || (list->object.type != OBJECT_CONS)) {
        ERROR("NOT A LIST, FFS!");
        return 0;
    }

    object_cons_t *tail = list;
    int i = 0;

    while((tail = (object_cons_t *) cdr(tail)) != NULL)
        i++;

    return i;
}

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// PRINT ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static const char *print_object(object_t * o) {
    if(o == NULL)
        return "NIL";

    switch (o->type) {
    case OBJECT_ERROR:
        return "ERR";
    case OBJECT_ATOM:
        return print_atom((object_atom_t *) o);
    case OBJECT_CONS:
        return print_cons((object_cons_t *) o);
    default:
        ERROR("print_object: unknwon object of type #%d", o->type);
    }

    return NULL;
}

static const char *print_atom(object_atom_t * object) {
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

static const char *print_cons(object_cons_t * list) {
    const size_t len = 255;
    char *s = ALLOC(len * sizeof(char) + 1);
    size_t si = 0;

    if((list == NULL) || (car(list) == NULL)) {
        strcpy(s, "NIL");
        return s;
    }

    while(car(list) != NULL) {
        if(si > len) {
            ERROR("si > len");
            free(s);
            return NULL;
        }

        const char *n = print_object(car(list));

        char *restrict start = s + si;
        const size_t end = len - si;

        si += snprintf(start, end, "%s%s", si ? " " : "(", n);

        list = (object_cons_t *) cdr(list);

        if((list == NULL) || (list->object.type != OBJECT_CONS))
            break;
    };

    if((list != NULL) && (list->object.type != OBJECT_CONS)) {
        char *restrict start = s + si;
        const size_t end = len - si;

        si += snprintf(start, end, " . %s", print_object((object_t *) list));
    }

    snprintf(s + si, len - si, ")");

    return s;
}

static object_atom_t *object_atom_new(atom_t * a) {
    object_atom_t *o = (object_atom_t *) object_new(OBJECT_ATOM);

    o->atom = a;

    return o;
}

static object_cons_t *object_cons_new(object_t * car, object_t * cdr) {
    object_cons_t *cons = (object_cons_t *) object_new(OBJECT_CONS);

    cons->car = car;
    cons->cdr = cdr;

    return cons;
}

static object_function_t *object_function_new(void) {
    return (object_function_t *) object_new(OBJECT_FUNCTION);
}

static object_t *object_new(object_type_t type) {
    size_t sz = 0;

    switch (type) {
    case OBJECT_ATOM:
        sz = sizeof(object_atom_t);
        break;
    case OBJECT_CONS:
        sz = sizeof(object_cons_t);
        break;
    case OBJECT_FUNCTION:
        sz = sizeof(object_function_t);
        break;
    case OBJECT_ERROR:
        ERROR("object_new: unknwon error");
    default:
        ERROR("object_new: unknown type");
        exit(EXIT_FAILURE);
    }

    object_t *object = ALLOC(sz);

    object->type = type;

    return object;
}

static void *ALLOC(const size_t sz) {
    void *o = calloc(1, sz);

    if(o == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    return o;
}

#if 0
static int debug(const char *file, const int line, const char *fmt, ...) {
    va_list ap;

    const size_t len = 256;
    char *s = calloc(len, sizeof(char));

    va_start(ap, fmt);
    vsnprintf(s, len, fmt, ap);
    va_end(ap);

    return fprintf(stderr, "debug[%s:%d] - %s\n", file, line, s);
}
#endif

static int error(const char *file, const int line, const char *fmt, ...) {
    va_list ap;

    const size_t len = 256;
    char *s = calloc(len, sizeof(char));

    va_start(ap, fmt);
    vsnprintf(s, len, fmt, ap);
    va_end(ap);

    return fprintf(stderr, "error[%s:%d] - %s\n", file, line, s);
}

static void panic(const char *file, const int line, const char *fmt, ...) {
    va_list ap;

    const size_t len = 256;
    char *s = calloc(len, sizeof(char));

    va_start(ap, fmt);
    vsnprintf(s, len, fmt, ap);
    va_end(ap);

    fprintf(stderr, "panic[%s:%d] - %s\n", file, line, s);

    exit(EXIT_FAILURE);
}
