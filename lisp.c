#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "lisp.h"
#include "lisp_print.h"
#include "lexer.h"
#include "list.h"

static object_t *object_new(object_type_t type);
static object_cons_t *object_cons_new(object_t *, object_t *);
static object_function_t *object_function_new(void);
static object_lambda_t *object_lambda_new(object_cons_t *, object_t *);
static object_integer_t *object_integer_new(int);
static object_string_t *object_string_new(char *, size_t);
static object_symbol_t *object_symbol_new(char *);

static object_t *read_object(lexer_t *);
static object_string_t *read_string(lexer_token_t *);
static object_integer_t *read_integer(lexer_token_t *);
static object_symbol_t *read_symbol(lexer_token_t *);

static object_cons_t *read_list(lexer_t *);

static object_t *eval_object(lisp_t *, lisp_env_t *, object_t *);
static object_t *eval_list(lisp_t *, lisp_env_t *, object_cons_t *);
static object_t *eval_symbol(lisp_t *, lisp_env_t *, object_symbol_t *);

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
    return eval_object(l, l->env, sexpr->object);
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

static object_t *C_atom(lisp_t * l, lisp_env_t * env __attribute__ ((unused)),
                        object_cons_t * args) {

    return atom(l, eval_object(l, env, first(args)));
}

static object_t *C_quote(lisp_t * l __attribute__ ((unused)),
                         lisp_env_t * env __attribute__ ((unused)),
                         object_cons_t * args) {

    return first(args);
}

static object_t *C_eq(lisp_t * l, lisp_env_t * env __attribute__ ((unused)),
                      object_cons_t * args) {

    return eq(l, eval_object(l, env, first(args)),
              eval_object(l, env, second(args)));
}

static object_t *C_cond(lisp_t * l, lisp_env_t * env, object_cons_t * args) {
    return cond(l, env, args);
}

static object_t *C_cons(lisp_t * l, lisp_env_t * env, object_cons_t * args) {
    return (object_t *) cons(eval_object(l, env, first(args)),
                             eval_object(l, env, second(args)));
}

static object_t *C_car(lisp_t * l, lisp_env_t * env, object_cons_t * args) {
    return car((object_cons_t *) eval_object(l, env, first(args)));
}

static object_t *C_cdr(lisp_t * l, lisp_env_t * env, object_cons_t * args) {
    return cdr((object_cons_t *) eval_object(l, env, first(args)));
}

static object_t *C_label(lisp_t * l, lisp_env_t * env, object_cons_t * args) {
    return label(l, env, (object_symbol_t *) first(args), second(args));
}

static object_t *C_lambda(lisp_t * l
                          __attribute__ ((unused)), lisp_env_t * env
                          __attribute__ ((unused)), object_cons_t * args) {

    return lambda((object_cons_t *) first(args), second(args));
}

static object_t *C_assoc(lisp_t * l, lisp_env_t * env
                         __attribute__ ((unused)), object_cons_t * args) {

    return assoc(l, eval_object(l, env, first(args)),
                 (object_cons_t *) eval_object(l, env, second(args)));
}

static object_t *eval_lambda(lisp_t * l, lisp_env_t * env,
                             object_lambda_t * lm) {

    if(lm == NULL)
        PANIC("eval_lambda: lambda is null");

    DEBUG("eval_lambda[%s, %s]", print_cons(env->labels),
          lisp_print((object_t *) lm));

    object_t *r = eval_object(l, env, lm->expr);

    DEBUG(" result: %s", lisp_print(r));

    return r;
}

static object_t *eval_object(lisp_t * l, lisp_env_t * env, object_t * object) {
    if(object == NULL)
        return NULL;

    switch (object->type) {
    case OBJECT_CONS:
        return eval_list(l, env, (object_cons_t *) object);
    case OBJECT_SYMBOL:
        return eval_symbol(l, env, (object_symbol_t *) object);
    case OBJECT_STRING:
    case OBJECT_INTEGER:
        return object;
    case OBJECT_LAMBDA:
        return eval_lambda(l, env, (object_lambda_t *) object);
    case OBJECT_FUNCTION:
        PANIC("Trying to evaluate function!");
    case OBJECT_ERROR:
        ERROR("eval_object: unexpected error");
    }

    ERROR("eval_object: unhandled object %s", lisp_print(object));
    exit(EXIT_FAILURE);
}

static object_t *eval_symbol(lisp_t * l, lisp_env_t * env,
                             object_symbol_t * sym) {

    if(eq(l, (object_t *) sym, (object_t *) object_symbol_new("NIL")))
        return NULL;

    if(eq(l, (object_t *) sym, l->t))
        return l->t;

    while(env != NULL) {
        object_t *obj = assoc(l, (object_t *) sym, env->labels);

        if(obj != NULL)
            return obj;

        env = env->outer;
    }

    PANIC("eval_symbol[_, %s] - no such symbol in environment",
          lisp_print((object_t *) sym));

    return NULL;
}

static lisp_env_t *lisp_env_new(lisp_env_t * outer, object_cons_t * labels) {
    lisp_env_t *env = calloc(1, sizeof(lisp_env_t));

    env->outer = outer;
    env->labels = labels;

    return env;
}

static object_t *eval_list(lisp_t * l, lisp_env_t * env, object_cons_t * list) {
    if((list == NULL) || (car(list) == NULL))
        return NULL;

    if(car(list) == NULL)
        PANIC("eval_list: function is NIL");

    object_t *prefix = eval_object(l, env, car(list));

    if(!atom(l, prefix)) {
        PANIC("eval_list: prefix must be atom, not %s, in %s",
              lisp_print(prefix), lisp_print((object_t *) list));
    }

    if(prefix == NULL) {
        PANIC("eval_list[_, %s] - prefix is NIL",
              lisp_print((object_t *) list));
    }

    if(prefix->type == OBJECT_FUNCTION) {
        object_function_t *fun = (object_function_t *) prefix;

        if((fun->args != -1) && (fun->args != (list_length(list) - 1))) {
            PANIC("function %s takes %d arguments",
                  lisp_print((object_t *) prefix), fun->args);
        }

        // TODO remove args, should be in env - like with lambdas
        return (*fun->fptr) (l, env, (object_cons_t *) cdr(list));
    }

    DEBUG("eval_list[_, _, %s]", print_cons(list));

    if(prefix->type == OBJECT_LAMBDA) {
        DEBUG("eval_list: LAMBDA!");

        object_lambda_t *lm = (object_lambda_t *) prefix;

        DEBUG(" args: %s, list: %s", print_cons(lm->args),
              lisp_print(cdr(list)));

        object_cons_t *args = (object_cons_t *) cdr(list);

#if 0
        object_cons_t *labels = pair(l, lm->args, args);
#else
        object_cons_t *labels = NULL;

        while(args != NULL) {
            object_t *lbl = eval_object(l, env, car(args));

            DEBUG(" label %s => %s", lisp_print(car(args)), lisp_print(lbl));

            DEBUG(" 1 labels: %s", print_cons(labels));
            labels = object_cons_new(lbl, (object_t *) labels);
            DEBUG(" 2 labels: %s", print_cons(labels));

            args = (object_cons_t *) cdr(args);
        }
#endif

        DEBUG(" 3 labels: %s", print_cons(labels));
        lisp_env_t *e = lisp_env_new(env, labels);

        return eval_lambda(l, e, lm);
    }

    PANIC("ARE WE DONE HERE?");
    (void) pair(l, NULL, NULL);

    if(((object_symbol_t *) prefix)->object.type != OBJECT_SYMBOL) {
        PANIC("eval_list: expecting symbol, not %s",
              lisp_print((object_t *) prefix));
    }

    object_symbol_t *fun_symbol = (object_symbol_t *) prefix;

    PANIC("TODO, next line!");
    //object_function_t *fun = (object_function_t *) assoc(l, prefix, l->env);
    object_function_t *fun = NULL;

    if(fun->object.type != OBJECT_FUNCTION) {
        PANIC("eval_list: unknown function name %s in %s", fun_symbol->name,
              lisp_print((object_t *) list));
    }

    if((fun->args != -1) && (fun->args != (list_length(list) - 1))) {
        PANIC("function %s takes %d arguments", lisp_print(prefix),
              fun->args);
    }

    // TODO remove args, should be in env
    object_t *r = (*fun->fptr) (l, lisp_env_new(env, NULL),
                                (object_cons_t *) cdr(list));

    return r;
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// BUILT-IN ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Return true if OBJECT is anything other than a CONS
object_t *atom(lisp_t * l, object_t * object) {
    if((object == NULL) || (object->type != OBJECT_CONS))
        return l->t;

    return NULL;
}

// apply a to list ==> (a . b)   -- b MUST be a list
object_cons_t *cons(object_t * a, object_t * b) {
    object_cons_t *cons = object_cons_new(a, b);

    return cons;
}

object_t *car(object_cons_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->object.type != OBJECT_CONS) {
        PANIC("car: object is not a list: %s (%d)\n",
              lisp_print((object_t *) cons), cons->object.type);
    }

    return cons->car;
}

object_t *cdr(object_cons_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->object.type != OBJECT_CONS)
        PANIC("cdr: object is not a list: %s\n",
              lisp_print((object_t *) cons));

    return cons->cdr;
}

object_t *cond(lisp_t * l, lisp_env_t * env, object_cons_t * list) {
    if(atom(l, (object_t *) list))
        return NULL;

    object_t *test = car((object_cons_t *) car(list));
    object_t *res = car((object_cons_t *) cdr((object_cons_t *) car(list)));

    if(eq(l, eval_object(l, env, test), l->t))
        return res;

    return cond(l, env, (object_cons_t *) cdr(list));
}

object_t *eq(lisp_t * l, object_t * a, object_t * b) {
    if(a == b)
        return l->t;

    if((a == NULL) || (b == NULL))
        return NULL;

    if(a->type != b->type)
        return NULL;

    if(a->type == OBJECT_ERROR)
        PANIC("eq(%s, %s) - OBJECT_ERROR", lisp_print(a), lisp_print(b));

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

object_t *first(object_cons_t * list) {
    if((list == NULL) || (list->object.type != OBJECT_CONS))
        return NULL;

    return car(list);
}

object_t *second(object_cons_t * list) {
    if((list == NULL) || (list_length(list) < 2))
        return NULL;

    return car((object_cons_t *) cdr(list));
}

#if 0
object_t *third(object_cons_t * list) {
    if((list == NULL) || (list_length(list) < 3))
        return NULL;

    return car((object_cons_t *) cdr((object_cons_t *) cdr(list)));
}
#endif

// TODO add test
object_t *assoc(lisp_t * l, object_t * x, object_cons_t * list) {
    if(atom(l, (object_t *) list))
        return NULL;

#if 0
    DEBUG("assoc[_, %s, %s]", lisp_print(x), print_cons(list));
    DEBUG(" %s vs %s", lisp_print(x), lisp_print(car(list)));
#endif

    if(eq(l, x, car((object_cons_t *) car(list))))
        return car((object_cons_t *) cdr((object_cons_t *) car(list)));

    return assoc(l, x, (object_cons_t *) cdr(list));
}

object_cons_t *pair(lisp_t * l, object_cons_t * k, object_cons_t * v) {
    if((k == NULL) && (v == NULL))
        return NULL;

    DEBUG("pair[%s@%p, %s@%p]", print_cons(k), k, print_cons(v), v);

    if(atom(l, (object_t *) k) || atom(l, (object_t *) v)) {
        PANIC("pair[%s, %s] - Atom!", lisp_print((object_t *) k),
              lisp_print((object_t *) v));
    }

    object_cons_t *tail = pair(l, (object_cons_t *) cdr(k),
                               (object_cons_t *) cdr(v));
    object_cons_t *head = object_cons_new((object_t *) car(k),
                                          (object_t *)
                                          object_cons_new((object_t *) car(v),
                                                          NULL));

    return object_cons_new((object_t *) head, (object_t *) tail);
}

object_t *label(lisp_t * l, lisp_env_t * env, object_symbol_t * sym,
                object_t * obj) {
#if 0
    WARN("label(_, %s, %s)", lisp_print((object_t *) sym), lisp_print(obj));
#endif

    lisp_env_t *envi = env;

    while(envi != NULL) {
        if(assoc(l, (object_t *) sym, envi->labels)) {
            WARN("label: redefining label %s", lisp_print((object_t *) sym));
        }

        envi = envi->outer;
    }

    if(assoc(l, (object_t *) sym, env->labels)) {
        PANIC("label(_, %s, %s) - redefinition!",
              lisp_print((object_t *) sym), lisp_print(obj));
    }

    object_cons_t *kv = object_cons_new((object_t *) sym,
                                        (object_t *) object_cons_new(obj,
                                                                     NULL));

    env->labels = object_cons_new((object_t *) kv, (object_t *) env->labels);

    return obj;
}

object_t *lambda(object_cons_t * args, object_t * expr) {
    DEBUG("lambda[_, %s, %s]", print_cons(args), lisp_print(expr));

    return (object_t *) object_lambda_new(args, expr);
}

int list_length(object_cons_t * list) {
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

static object_cons_t *object_cons_new(object_t * car, object_t * cdr) {
    object_cons_t *cons = (object_cons_t *) object_new(OBJECT_CONS);

    cons->car = car;
    cons->cdr = cdr;

    return cons;
}

static object_function_t *object_function_new(void) {
    return (object_function_t *) object_new(OBJECT_FUNCTION);
}

static object_lambda_t *object_lambda_new(object_cons_t * args,
                                          object_t * expr) {

    object_lambda_t *l = (object_lambda_t *) object_new(OBJECT_LAMBDA);

    l->args = args;
    l->expr = expr;

    return l;
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
    case OBJECT_LAMBDA:
        return sizeof(object_lambda_t);
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

void *ALLOC(const size_t sz) {
    void *o = calloc(1, sz);

    if(o == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    return o;
}

int logger(const char *lvl, const char *file, const int line,
           const char *fmt, ...) {

    va_list ap;

    const size_t len = 256;
    char *s = calloc(len, sizeof(char));

    va_start(ap, fmt);
    vsnprintf(s, len, fmt, ap);
    va_end(ap);

    return fprintf(stderr, "%s[%s:%03d] - %s\n", lvl, file, line, s);
}

#define ADD_FUN(l, e, name, fun, argc) do { \
    object_function_t *f = object_function_new(); \
    f->fptr = fun; f->args = argc; \
    label(l, e, object_symbol_new(name), (object_t *) f); } while(0);

lisp_t *lisp_new() {
    lisp_t *l = calloc(1, sizeof(lisp_t));

    l->env = lisp_env_new(NULL, NULL);

    object_symbol_t *t = object_symbol_new("T");

    l->t = (object_t *) t;
    label(l, l->env, t, l->t);

    ADD_FUN(l, l->env, "CONS", C_cons, 2);
    ADD_FUN(l, l->env, "CAR", C_car, 1);
    ADD_FUN(l, l->env, "CDR", C_cdr, 1);
    ADD_FUN(l, l->env, "EQ", C_eq, 2);
    ADD_FUN(l, l->env, "COND", C_cond, -1);
    ADD_FUN(l, l->env, "QUOTE", C_quote, 1);
    ADD_FUN(l, l->env, "ATOM", C_atom, 1);
    ADD_FUN(l, l->env, "LABEL", C_label, 2);
    ADD_FUN(l, l->env, "LAMBDA", C_lambda, 2);

    ADD_FUN(l, l->env, "ASSOC", C_assoc, 2);

    return l;
}

/* TODO:
 *   - Arguments (in definitions) should be defined by lists, not int count
 *   - Split out print_*, read_* and object_*
 */
