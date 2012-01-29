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
    logger("debug", __FILE__, __LINE__, f, ##args); } while(0);

#define WARN(f, args...) do { \
    logger("warning", __FILE__, __LINE__, f, ##args); } while(0);

#define ERROR(f, args...) do { \
    logger("error", __FILE__, __LINE__, f, ##args); } while(0);

#define PANIC(f, args...) do { \
    logger("panic", __FILE__, __LINE__, f, ##args); \
    exit(EXIT_FAILURE); } while(0);

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

static int logger(const char *, const char *, const int, const char *, ...);

static void *ALLOC(size_t);

static object_t *object_new(object_type_t type);
static object_cons_t *object_cons_new(object_t *, object_t *);
static object_function_t *object_function_new(void);
static object_integer_t *object_integer_new(int);
static object_string_t *object_string_new(char *, size_t);
static object_symbol_t *object_symbol_new(char *);

static object_t *read_object(lexer_t *);
static object_string_t *read_string(lexer_token_t *);
static object_integer_t *read_integer(lexer_token_t *);
static object_symbol_t *read_symbol(lexer_token_t *);

static object_cons_t *read_list(lexer_t *);

static object_t *eval_object(lisp_t *, object_t *);
static object_t *eval_list(lisp_t *, object_cons_t *);
static object_t *eval_symbol(lisp_t *, object_symbol_t *);

static const char *print_integer(object_integer_t *);
static const char *print_string(object_string_t *);
static const char *print_cons(object_cons_t *);
static const char *print_object(object_t *);
static const char *print_cons(object_cons_t *);

static object_cons_t *cons(object_t *, object_t *);
static object_t *car(object_cons_t *);
static object_t *cdr(object_cons_t *);
static object_t *atom(lisp_t *, object_t *);
static object_t *eq(lisp_t *, object_t *, object_t *);
static object_t *cond(lisp_t *, object_cons_t *);
static object_t *first(object_cons_t *);
static object_t *second(object_cons_t *);
static object_t *label(lisp_t *, object_symbol_t *, object_t *);
static object_t *assoc(lisp_t *, object_t *, object_cons_t *);

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

object_t *lisp_eval(lisp_t * l, sexpr_t * sexpr) {
    return eval_object(l, sexpr->object);
}

// Render object to string. Client must free() returned value.
const char *lisp_print(object_t * object) {
    return print_object(object);
}

static object_t *read_object(lexer_t * lexer) {
    object_t *object;

    lexer_token_t *token = lexer_expect(lexer, TOKEN_ATOM);

    if(token == NULL)
        return (object_t *) read_list(lexer);

    object = (object_t *) read_integer(token);
    if(object != NULL)
        return object;

    object = (object_t *) read_string(token);
    if(object != NULL)
        return object;

    object = (object_t *) read_symbol(token);
    if(object != NULL)
        return object;

    return NULL;
}

static object_symbol_t *read_symbol(lexer_token_t * token) {
    if(token->type != TOKEN_ATOM)
        return NULL;

    object_symbol_t *symbol = object_symbol_new(calloc(1, token->len + 1));

    if(symbol->name == NULL)
        PANIC("read_symbol: calloc");

    strncpy((char *restrict) symbol->name, token->text, token->len);

    return symbol;
}

static object_string_t *read_string(lexer_token_t * token) {
    if((token == NULL) || (token->type != TOKEN_ATOM) || (token->len == 0))
        return NULL;

    // String tokens must be surrounded by quotes
    if((token->text[0] != '"') || (token->text[token->len] != '"'))
        return NULL;

    object_string_t *o = object_string_new(calloc(token->len, sizeof(char)),
                                           token->len - 1);

    if(o->string == NULL)
        PANIC("read_string: calloc");

    strncpy((char *restrict) o->string, token->text + 1, token->len + 2);

    return o;
}

static object_integer_t *read_integer(lexer_token_t * token) {
    if((token == NULL) || (token->type != TOKEN_ATOM) || (token->len == 0))
        return NULL;

    // Check token is all numbers [0..9]
    for(size_t i = 0; i < token->len; i++) {
        if(!isdigit(token->text[i]))
            return NULL;
    }

    return object_integer_new(atoi(token->text));
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

static object_t *C_atom(lisp_t * l, object_cons_t * args) {
    return atom(l, eval_object(l, first(args)));
}

static object_t *C_quote(lisp_t * l __attribute__ ((unused)),
                         object_cons_t * args) {

    return first(args);
}

static object_t *C_eq(lisp_t * l, object_cons_t * args) {
    return eq(l, eval_object(l, first(args)), eval_object(l, second(args)));
}

static object_t *C_cond(lisp_t * l, object_cons_t * args) {
    return cond(l, args);
}

static object_t *C_cons(lisp_t * l, object_cons_t * args) {
    return (object_t *) cons(eval_object(l, first(args)),
                             eval_object(l, second(args)));
}

static object_t *C_car(lisp_t * l, object_cons_t * args) {
    return car((object_cons_t *) eval_object(l, first(args)));
}

static object_t *C_cdr(lisp_t * l, object_cons_t * args) {
    return cdr((object_cons_t *) eval_object(l, first(args)));
}

static object_t *C_label(lisp_t * l, object_cons_t * args) {
    return label(l, (object_symbol_t *) first(args), second(args));
}

static object_t *C_assoc(lisp_t * l, object_cons_t * args) {
//static object_t *assoc(lisp_t *, object_t *, object_cons_t *);
    return assoc(l, eval_object(l, first(args)),
                 (object_cons_t *) eval_object(l, second(args)));
}

static object_t *eval_object(lisp_t * l, object_t * object) {
    if(object == NULL)
        return NULL;

    switch (object->type) {
    case OBJECT_CONS:
        return eval_list(l, (object_cons_t *) object);
    case OBJECT_SYMBOL:
        return eval_symbol(l, (object_symbol_t *) object);
    case OBJECT_STRING:
    case OBJECT_INTEGER:
        return object;
    case OBJECT_FUNCTION:
        PANIC("Trying to evaluate function!");
    case OBJECT_ERROR:
        ERROR("eval_object: unexpected error");
    }

    ERROR("eval_object: unhandled object %s", print_object(object));
    exit(EXIT_FAILURE);
}

static object_t *eval_symbol(lisp_t * l, object_symbol_t * sym) {
    if(eq(l, (object_t *) sym, (object_t *) object_symbol_new("NIL")))
        return NULL;

    object_t *o = assoc(l, (object_t *) sym, l->env);

    if(o == NULL) {
        PANIC("eval_symbol[_, %s] - no such symbol in %s",
              print_object((object_t *) sym), print_cons(l->env));
    }

    return o;
}

static object_t *eval_list(lisp_t * l, object_cons_t * list) {
    if((list == NULL) || (car(list) == NULL))
        return NULL;

    if(car(list) == NULL)
        PANIC("eval_list: function is NIL");

    object_t *prefix = eval_object(l, car(list));

    if(!atom(l, prefix)) {
        PANIC("eval_list: prefix must be atom, not %s, in %s",
              print_object(prefix), print_cons(list));
    }

    if(prefix == NULL)
        PANIC("eval_list[_, %s] - prefix is NIL", print_cons(list));

    if(prefix->type == OBJECT_FUNCTION) {
        object_function_t *fun = (object_function_t *) prefix;

        if((fun->args != -1) && (fun->args != (list_length(list) - 1))) {
            PANIC("function %s takes %d arguments", print_object(prefix),
                  fun->args);
        }

        // TODO remove args, should be in env
        return (*fun->fptr) (l, (object_cons_t *) cdr(list));
    }

    PANIC("ARE WE DONE HERE?");

    if(((object_symbol_t *) prefix)->object.type != OBJECT_SYMBOL) {
        PANIC
            ("eval_list: expecting symbol object as first element of list, not %s",
             print_object((object_t *) prefix));
    }

    object_symbol_t *fun_symbol = (object_symbol_t *) prefix;
    object_function_t *fun = (object_function_t *) assoc(l, prefix, l->env);

    if((fun == NULL) || (fun->object.type != OBJECT_FUNCTION)) {
        PANIC("eval_list: unknown function name %s in %s", fun_symbol->name,
              print_cons(list));
    }

    if((fun->args != -1) && (fun->args != (list_length(list) - 1))) {
        PANIC("function %s takes %d arguments", print_object(prefix),
              fun->args);
    }

    // TODO remove args, should be in env
    object_t *r = (*fun->fptr) (l, (object_cons_t *) cdr(list));

    return r;
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// BUILT-IN ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Return true if OBJECT is anything other than a CONS
static object_t *atom(lisp_t * l, object_t * object) {
    if((object == NULL) || (object->type != OBJECT_CONS))
        return l->t;

    return NULL;
}

// apply a to list ==> (a . b)   -- b MUST be a list
static object_cons_t *cons(object_t * a, object_t * b) {
    object_cons_t *cons = object_cons_new(a, b);

    return cons;
}

static object_t *car(object_cons_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->object.type != OBJECT_CONS) {
        ERROR("car: object is not a list: %s (%d)\n",
              print_object((object_t *) cons), cons->object.type);
        exit(EXIT_FAILURE);
    }

    return cons->car;
}

static object_t *cdr(object_cons_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->object.type != OBJECT_CONS)
        PANIC("cdr: object is not a list: %s\n",
              print_object((object_t *) cons));

    return cons->cdr;
}

static object_t *cond(lisp_t * l, object_cons_t * list) {
    if(atom(l, (object_t *) list))
        return NULL;

    object_t *test = car((object_cons_t *) car(list));
    object_t *res = car((object_cons_t *) cdr((object_cons_t *) car(list)));

    if(eq(l, eval_object(l, test), l->t))
        return res;

    return cond(l, (object_cons_t *) cdr(list));
}

static object_t *eq(lisp_t * l, object_t * a, object_t * b) {
    if(a == b)
        return l->t;

    if((a == NULL) || (b == NULL))
        return NULL;

    if(a->type != b->type)
        return NULL;

    if(a->type == OBJECT_ERROR)
        PANIC("eq(%s, %s) - OBJECT_ERROR", print_object(a), print_object(b));

    if(a->type == OBJECT_CONS)
        return NULL;

    if(a->type == OBJECT_INTEGER) {
        if(((object_integer_t *) a)->number ==
           ((object_integer_t *) a)->number) {

            return l->t;
        }

        return NULL;
    }

    if(a->type == OBJECT_STRING) {
        if(0 ==
           strncmp(((object_string_t *) a)->string,
                   ((object_string_t *) a)->string,
                   ((object_string_t *) a)->len)) {

            return l->t;
        }

        return NULL;
    }

    if(a->type == OBJECT_SYMBOL) {
        if(0 == strcmp(((object_symbol_t *) a)->name,
                       ((object_symbol_t *) b)->name)) {

            return l->t;
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

// TODO add test
static object_t *assoc(lisp_t * l, object_t * x, object_cons_t * list) {
    if(atom(l, (object_t *) list))
        return NULL;

#if 0
    DEBUG("assoc[_, %s, %s]", print_object(x), print_cons(list));
    DEBUG(" %s vs %s", print_object(x), print_object(car(list)));
#endif

    if(eq(l, x, car((object_cons_t *) car(list))))
        return car((object_cons_t *) cdr((object_cons_t *) car(list)));

    return assoc(l, x, (object_cons_t *) cdr(list));
}

static object_t *pair(object_t * k, object_t * v) {
    return (object_t *) object_cons_new(k, (object_t *) object_cons_new(v,
                                                                        NULL));
}

static object_t *label(lisp_t * l, object_symbol_t * sym, object_t * obj) {
#if 0
    WARN("label(_, %s, %s)", print_object((object_t *) sym),
         print_object(obj));
#endif

    if(assoc(l, (object_t *) sym, l->env)) {
        PANIC("label(_, %s, %s) - redefinition!",
              print_object((object_t *) sym), print_object(obj));
    }

    l->env = object_cons_new(pair((object_t *) sym, obj),
                             (object_t *) l->env);

    return obj;
}

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
    case OBJECT_CONS:
        return print_cons((object_cons_t *) o);
    case OBJECT_INTEGER:
        return print_integer((object_integer_t *) o);
    case OBJECT_FUNCTION:
        return "#<Function>";   // TODO
    case OBJECT_STRING:
        return print_string((object_string_t *) o);
    case OBJECT_SYMBOL:
        return ((object_symbol_t *) o)->name;
    }

    ERROR("print_object: unknwon object of type #%d", o->type);

    return NULL;
}

static const char *print_string(object_string_t * str) {
    const size_t len = str->len + 3;
    char *s = calloc(len, sizeof(char));

    if(snprintf(s, len, "\"%s\"", str->string) == 0)
        PANIC("print_string: could not write string %s", str->string);

    return s;
}

static const char *print_integer(object_integer_t * integer) {
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
        strcpy(s, "(NIL)");
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

static object_cons_t *object_cons_new(object_t * car, object_t * cdr) {
    object_cons_t *cons = (object_cons_t *) object_new(OBJECT_CONS);

    cons->car = car;
    cons->cdr = cdr;

    return cons;
}

static object_function_t *object_function_new(void) {
    return (object_function_t *) object_new(OBJECT_FUNCTION);
}

static object_integer_t *object_integer_new(int num) {
    object_integer_t *o = (object_integer_t *) object_new(OBJECT_INTEGER);

    o->number = num;
    return o;
}

static object_string_t *object_string_new(char *s, size_t n) {
    object_string_t *o = (object_string_t *) object_new(OBJECT_STRING);

    o->string = s;
    o->len = n;

    return o;
}

static object_symbol_t *object_symbol_new(char *s) {
    object_symbol_t *o = (object_symbol_t *) object_new(OBJECT_SYMBOL);

    o->name = s;

    return o;
}

static size_t object_sz(object_type_t type) {
    switch (type) {
    case OBJECT_CONS:
        return sizeof(object_cons_t);
    case OBJECT_FUNCTION:
        return sizeof(object_function_t);
    case OBJECT_INTEGER:
        return sizeof(object_integer_t);
    case OBJECT_STRING:
        return sizeof(object_string_t);
    case OBJECT_SYMBOL:
        return sizeof(object_symbol_t);
    case OBJECT_ERROR:
        PANIC("object_new: unknwon error");
    }

    ERROR("object_new: unknown type");
    exit(EXIT_FAILURE);
}

static object_t *object_new(object_type_t type) {
    object_t *object = ALLOC(object_sz(type));

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

static int logger(const char *lvl, const char *file, const int line,
                  const char *fmt, ...) {

    va_list ap;

    const size_t len = 256;
    char *s = calloc(len, sizeof(char));

    va_start(ap, fmt);
    vsnprintf(s, len, fmt, ap);
    va_end(ap);

    return fprintf(stderr, "%s[%s:%03d] - %s\n", lvl, file, line, s);
}

#define ADD_FUN(l, name, fun, argc) do { \
    object_function_t *f = object_function_new(); \
    f->fptr = fun; f->args = argc; \
    label(l, object_symbol_new(name), (object_t *) f); } while(0);

lisp_t *lisp_new() {
    lisp_t *l = calloc(1, sizeof(lisp_t));

    l->env = NULL;

    object_symbol_t *t = object_symbol_new("T");

    l->t = (object_t *) t;
    label(l, t, l->t);

    ADD_FUN(l, "CONS", C_cons, 2);
    ADD_FUN(l, "CAR", C_car, 1);
    ADD_FUN(l, "CDR", C_cdr, 1);
    ADD_FUN(l, "EQ", C_eq, 2);
    ADD_FUN(l, "COND", C_cond, -1);
    ADD_FUN(l, "QUOTE", C_quote, 1);
    ADD_FUN(l, "ATOM", C_atom, 1);
    ADD_FUN(l, "LABEL", C_label, 2);

    ADD_FUN(l, "ASSOC", C_assoc, 2);

    return l;
}
