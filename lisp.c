#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "lisp.h"
#include "lisp_print.h"
#include "lisp_eval.h"
#include "lexer.h"
#include "list.h"

static object_t *read_object(lexer_t *);
static object_t *read_string(lexer_token_t *);
static object_t *read_integer(lexer_token_t *);
static object_t *read_symbol(lexer_token_t *);

static object_t *object_new(object_type_t type);
static object_t *read_list(lexer_t *);

object_t *lisp_read(const char *s, size_t len) {
    lexer_t *lexer = lexer_init(s, len);

    if(lexer == NULL)
        return NULL;

    return read_object(lexer);
}

static object_t *read_object(lexer_t * lexer) {
    object_t *object;

    lexer_token_t *token = lexer_expect(lexer, TOKEN_ATOM);

    if(token == NULL)
        return read_list(lexer);

    object = read_integer(token);
    if(object != NULL)
        return object;

    object = read_string(token);
    if(object != NULL)
        return object;

    object = read_symbol(token);
    if(object != NULL)
        return object;

    return NULL;
}

static object_t *read_symbol(lexer_token_t * token) {
    if(token->type != TOKEN_ATOM)
        return NULL;

    object_symbol_t *symbol =
        (object_symbol_t *) object_symbol_new(calloc(1, token->len + 1));

    if(symbol->name == NULL)
        PANIC("read_symbol: calloc");

    strncpy((char *restrict) symbol->name, token->text, token->len);

    return (object_t *) symbol;
}

static object_t *read_string(lexer_token_t * token) {
    if((token == NULL) || (token->type != TOKEN_ATOM) || (token->len == 0))
        return NULL;

    // String tokens must be surrounded by quotes
    if((token->text[0] != '"') || (token->text[token->len] != '"'))
        return NULL;

    object_string_t *o = (object_string_t *)
        object_string_new(calloc(token->len, sizeof(char)),
                          token->len - 1);

    if(o->string == NULL)
        PANIC("read_string: calloc");

    strncpy((char *restrict) o->string, token->text + 1, token->len + 2);

    return (object_t *) o;
}

static object_t *read_integer(lexer_token_t * token) {
    if((token == NULL) || (token->type != TOKEN_ATOM) || (token->len == 0))
        return NULL;

    // Check token is all numbers [0..9]
    for(size_t i = 0; i < token->len; i++) {
        if(!isdigit(token->text[i]))
            return NULL;
    }

    return object_integer_new(atoi(token->text));
}

static object_t *read_list(lexer_t * lexer) {
    lexer_token_t *token = lexer_expect(lexer, TOKEN_LIST_START);

    if(token->type != TOKEN_LIST_START)
        PANIC("read_list - no TOKEN_LIST_START!");

    if((token == NULL) || (lexer_expect(lexer, TOKEN_LIST_END)))
        return NULL;

    object_t *list = object_cons_new(NULL, NULL);
    object_cons_t *pair = (object_cons_t *) list;

    while(NULL == lexer_expect(lexer, TOKEN_LIST_END)) {
        pair->car = read_object(lexer);
        pair->cdr = object_cons_new(NULL, NULL);

        pair = (object_cons_t *) pair->cdr;
    }

    return list;
}

lisp_env_t *lisp_env_new(lisp_env_t * outer, object_t * labels) {
    lisp_env_t *env = calloc(1, sizeof(lisp_env_t));

    env->outer = outer;
    env->labels = labels;

    return env;
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// BUILT-IN ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// Return true if OBJECT is anything other than a CONS
// TODO: atom should propably take *env also
object_t *atom(lisp_t * l, object_t * object) {
    if((object == NULL) || (object->type != OBJECT_CONS))
        return l->t;

    return NULL;
}

// apply a to list ==> (a . b)   -- b MUST be a list
object_t *cons(object_t * a, object_t * b) {
    return object_cons_new(a, b);
}

object_t *car(object_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->type != OBJECT_CONS) {
        PANIC("car: object is not a list: %s (%d)\n",
              lisp_print(cons), cons->type);
    }

    return ((object_cons_t *) cons)->car;
}

object_t *cdr(object_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->type != OBJECT_CONS)
        PANIC("cdr: object is not a list: %s\n", lisp_print(cons));

    return ((object_cons_t *) cons)->cdr;
}

object_t *cond(lisp_t * l, lisp_env_t * env, object_t * list) {
    if(atom(l, list))
        return NULL;

    object_t *test = car(car(list));
    object_t *res = car(cdr(car(list)));

    if(eq(l, lisp_eval(l, env, test), l->t))
        return res;

    return cond(l, env, cdr(list));
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

object_t *first(object_t * list) {
    if((list == NULL) || (list->type != OBJECT_CONS))
        return NULL;

    return car(list);
}

object_t *second(object_t * list) {
    if((list == NULL) || (list_length(list) < 2))
        return NULL;

    return car(cdr(list));
}

#if 0
object_t *third(object_cons_t * list) {
  if((list == NULL) || (list_length(list) < 3) iindent: Standard input: 600: Warning: Unterminated character constant indent: Standard input: 599: Warning: Unterminated character constant indent: Standard input: 598: Warning: Unterminated character constant indent: Standard input: 597: Warning: Unterminated character constant indent: Standard input: 596: Warning: Unterminated character constant indent: Standard input: 595: Warning: Unterminated character constant indent: Standard input: 594: Warning: Unterminated character constant indent: Standard input: 593: Warning: Unterminated character constant indent: Standard input: 592: Warning: Unterminated character constant indent: Standard input: 591: Warning: Unterminated character constant indent: Standard input: 588: Warning: Unterminated character constant indent: Standard input: 587: Warning: Unterminated character constant indent: Standard input: 586: Warning: Unterminated character constant indent: Standard input: 585: Warning: Unterminated character constant indent: Standard input: 584: Warning: Unterminated character constant indent: Standard input: 583: Warning: Unterminated character constant indent: Standard input: 582: Warning: Unterminated character constant indent: Standard input: 581: Warning: Unterminated character constant indent: Standard input: 580: Warning: Unterminated character constant indent: Standard input: 579: Warning: Unterminated character constant indent: Standard input: 578: Warning: Unterminated character constant indent: Standard input: 577: Warning: Unterminated character constant indent: Standard input: 576: Warning: Unterminated character constant indent: Standard input: 575: Warning: Unterminated character constant indent: Standard input: 574: Warning: Unterminated character constant indent: Standard input: 573: Warning: Unterminated character constant indent: Standard input: 572: Warning: Unterminated character constant indent: Standard input: 571: Warning: Unterminated character constant indent: Standard input: 570: Warning: Unterminated character constant indent: Standard input: 569: Warning: Unterminated character constant indent: Standard input: 568: Warning: Unterminated character constant indent: Standard input: 567: Warning: Unterminated character constant indent: Standard input: 566: Warning: Unterminated character constant indent: Standard input: 565: Warning: Unterminated character constant indent: Standard input: 564: Warning: Unterminated character constant indent: Standard input: 563: Warning: Unterminated character constant indent: Standard input: 562: Warning: Unterminated character constant indent: Standard input: 561: Warning: Unterminated character constant indent: Standard input: 559: Warning: Unterminated character constant ndent: Standard input: 557: Warning:Unterminated character
       constant)
        return NULL;

    return car((object_cons_t *) cdr((object_cons_t *) cdr(list)));
}
#endif

/* (defun assoc. (x y)
 *   (cond ((eq (caar y) x) (cadar y))
 *           ('t (assoc. x (cdr y)))))
 */
// TODO add test
object_t *assoc(lisp_t * l, object_t * x, object_t * o) {
    TRACE("assoc[_, %s, %s]", lisp_print(x), lisp_print(list));

    if(o == NULL)
        return NULL;

    if(o->type != OBJECT_CONS)
        PANIC("assoc: expected list");

    if(atom(l, o))
        return NULL;

    if(eq(l, x, car(car(o))))
        return car(cdr(car(o)));

    return assoc(l, x, cdr(o));
}

/*
(defun pair(x y)
 (cond((and(null x) (null y)) '())
              ((and (not (atom x)) (not (atom y)))
               (cons (list (car x) (car y))
                     (pair (cdr x) (cdr y))))))
                     */
object_t *pair(lisp_t * l, object_t * k, object_t * v) {
    TRACE("pair[%s@%p, %s@%p]", lisp_print(k), k, lisp_print(v), v);

    if((k == NULL) || (v == NULL))
        return NULL;

    if((k->type != OBJECT_CONS) || (k->type != OBJECT_CONS))
        PANIC("pair: expected cons'");

    if(atom(l, k) || atom(l, v))
        return NULL;

#if 0
    object_cons_t *tail = pair(l, (object_cons_t *) cdr(k),
                               (object_cons_t *) cdr(v));
    object_cons_t *head = object_cons_new((object_t *) car(k),
                                          (object_t *)
                                          object_cons_new((object_t *) car(v),
                                                          NULL));

    return object_cons_new((object_t *) head, (object_t *) tail);
#endif

    // TODO - make list(object_t *...) helper
    object_t *head = object_cons_new(car(k), object_cons_new(car(v), NULL));
    object_t *tail = pair(l, cdr(k), cdr(v));

    if(car(head) == NULL)
        head = NULL;

    if(car(tail) == NULL)
        tail = NULL;

    return object_cons_new(head, tail);
}

object_t *label(lisp_t * l, lisp_env_t * env, object_t * sym, object_t * obj) {
    TRACE("label(_, %s, %s)", lisp_print(sym), lisp_print(obj));

    lisp_env_t *envi = env;

    while(envi != NULL) {
        if(assoc(l, (object_t *) sym, envi->labels)) {
            WARN("label: redefining label %s", lisp_print((object_t *) sym));
        }

        envi = envi->outer;
    }

    object_t *kv = object_cons_new(sym, object_cons_new(obj, NULL));

    if(env == NULL) {
        if(l->env == NULL)
            PANIC("no environment!");

        env = l->env;
    }

    env->labels = object_cons_new(kv, env->labels);

    return obj;
}

object_t *lambda(object_t * args, object_t * expr) {
    DEBUG("lambda[_, %s, %s]", lisp_print((object_t *) args),
          lisp_print(expr));

    return (object_t *) object_lambda_new(args, expr);
}

int list_length(object_t * list) {
    if((list == NULL) || (list->type != OBJECT_CONS)) {
        ERROR("NOT A LIST, FFS!");
        return 0;
    }

    object_t *tail = list;
    int i = 0;

    while((tail = cdr(tail)) != NULL)
        i++;

    return i;
}

object_t *object_cons_new(object_t * car, object_t * cdr) {
    object_cons_t *cons = (object_cons_t *) object_new(OBJECT_CONS);

    cons->car = car;
    cons->cdr = cdr;

    return (object_t *) cons;
}

object_t *object_function_new(void) {
    return (object_t *) object_new(OBJECT_FUNCTION);
}

object_t *object_lambda_new(object_t * args, object_t * expr) {

    object_lambda_t *l = (object_lambda_t *) object_new(OBJECT_LAMBDA);

    l->args = args;
    l->expr = expr;

    return (object_t *) l;
}

object_t *object_integer_new(int num) {
    object_integer_t *o = (object_integer_t *) object_new(OBJECT_INTEGER);

    o->number = num;
    return (object_t *) o;
}

object_t *object_string_new(char *s, size_t n) {
    object_string_t *o = (object_string_t *) object_new(OBJECT_STRING);

    o->string = s;
    o->len = n;

    return (object_t *) o;
}

object_t *object_symbol_new(char *s) {
    object_symbol_t *o = (object_symbol_t *) object_new(OBJECT_SYMBOL);

    o->name = s;

    return (object_t *) o;
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

lisp_t *lisp_new() {
    lisp_t *l = calloc(1, sizeof(lisp_t));

    l->env = lisp_env_new(NULL, NULL);

    l->t = object_symbol_new("T");
    l->nil = object_symbol_new("NIL");

    return l;
}

/* TODO:
 *   - Arguments (in definitions) should be defined by lists, not int count
 *   - Split out print_*, read_* and object_*
 */
