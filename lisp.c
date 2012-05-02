#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "logger.h"
#include "lisp.h"
#include "lisp_print.h"
#include "lisp_eval.h"
#include "list.h"
#include "object.h"
#include "stream.h"
#include "builtin.h"

object_t *lisp_read(lisp_t * lisp, const char *s, size_t len) {
    TRACE("lisp_read[_, %s, %d]", s, len);

    object_t *stream = istream_mem(s, len);
    object_t *o = read(lisp, stream);

    return car(macroexpand(lisp, NULL, o));
}

lisp_env_t *lisp_env_new(lisp_env_t * outer, object_t * labels) {
    lisp_env_t *env = calloc(1, sizeof(lisp_env_t));

    env->outer = outer;
    env->labels = labels;

    return env;
}

lisp_env_t *lisp_env_pop(lisp_env_t * env) {
    if(env == NULL)
        return NULL;

    return env->outer;
}

object_t *lisp_env_resolv(lisp_t * l, lisp_env_t * env, object_t * x) {
    if(env == NULL)
        return NULL;

    TRACE("lisp_env_resolv[_, %s, %s]", lisp_print(env->labels),
          lisp_print(x));

    object_t *pairi = env->labels;

    while(pairi) {
        if(eq(l, car(car(pairi)), x))
            return car(pairi);

        pairi = cdr(pairi);
    }

    return lisp_env_resolv(l, env->outer, x);
}

/** Read list of objects from input-stream. */
static object_t *mread_list(lisp_t * l, char x, object_t * stream) {
    TRACE("mread_list[_, '%c', »%s«]", x, lisp_print(stream));

    if(x != '(')
        PANIC("mread_list cannot read non-list");

    object_t *o = read(l, stream);

    if(o == NULL)
        return NULL;

    object_t *list = cons(o, NULL);
    object_t *tail = NULL;

    while((o = read(l, stream))) {
        if(tail == NULL) {
            tail = ((object_cons_t *) list)->cdr = cons(o, NULL);
        }
        else {
            tail = ((object_cons_t *) tail)->cdr = cons(o, NULL);
        }

    }

    return list;
}

static object_t *mread_str(lisp_t * l, char x, object_t * stream) {
    l = l;

    if(x != '"')
        PANIC("mread_str cannot read non-string");

    size_t str_idx = 0, str_sz = 255;
    char *str = calloc(str_sz + 1, sizeof(char));

    while(!stream_eof(stream)) {
        if(str_idx > str_sz)
            PANIC("string overflow");

        char x = stream_read_char(stream);

        if(x == '"')
            break;

        str[str_idx++] = x;
    }

    return object_string_new(str, str_idx);
}

static object_t *mread_quote(lisp_t * l, char x, object_t * stream) {
    if(x != '\'')
        PANIC("mread_quote cannot read non-quote");

    return cons(object_symbol_new("QUOTE"), cons(read(l, stream), NULL));
}

static object_t *mread_unquote(lisp_t * l, char x, object_t * stream) {
    if(x != ',')
        PANIC("mread_unquote cannot read non-unquote");

    return cons(object_symbol_new("UNQUOTE"), cons(read(l, stream), NULL));
}

static object_t *mread_backquote(lisp_t * l, char x, object_t * stream) {
    TRACE("mread_backquote[_, _, _]");

    if(x != '`')
        PANIC("mread_backquote cannot read non-backquote");

    // register a temporary backquote-escape macro reader
    object_t *old_readtable = l->readtable;
    object_t *entry =
        cons(object_symbol_new(","), (object_t *) mread_unquote);
    l->readtable = cons(entry, l->readtable);

    // read elements
    object_t *list = NULL, *tail = NULL;
    object_t *elems = read(l, stream);

    while(elems) {
        object_t *o = NULL;

        if(!atom(l, car(elems))
           && eq(l, car(car(elems)), object_symbol_new("UNQUOTE"))) {
            o = car(cdr(car(elems)));
        }
        else {
            o = cons(object_symbol_new("QUOTE"), cons(car(elems), NULL));
        }

        if(list == NULL) {
            list = cons(o, NULL);
        }
        else if(tail == NULL) {
            tail = ((object_cons_t *) list)->cdr = cons(o, NULL);
        }
        else {
            tail = ((object_cons_t *) tail)->cdr = cons(o, NULL);
        }

        elems = cdr(elems);
    }

    l->readtable = old_readtable;

    return list;
}

static object_t *readtable_new(void) {
    object_t *readtable = NULL;
    object_t *entry = NULL;

    entry = cons(object_symbol_new("("), (object_t *) mread_list);
    readtable = cons(entry, readtable);

    entry = cons(object_symbol_new("\""), (object_t *) mread_str);
    readtable = cons(entry, readtable);

    entry = cons(object_symbol_new("\'"), (object_t *) mread_quote);
    readtable = cons(entry, readtable);

    entry = cons(object_symbol_new("`"), (object_t *) mread_backquote);
    readtable = cons(entry, readtable);

    return readtable;
}

object_t *atom_fw(lisp_t * l, object_t * args) {
    return atom(l, car(args));
}

object_t *eq_fw(lisp_t * l, object_t * args) {
    return eq(l, car(args), car(cdr(args)));
}

object_t *car_fw(lisp_t * l, object_t * args) {
    l = l;
    return car(car(args));
}

object_t *cdr_fw(lisp_t * l, object_t * args) {
    l = l;
    return cdr(car(args));
}

object_t *cons_fw(lisp_t * l, object_t * args) {
    l = l;
    return cons(car(args), car(cdr(args)));
}

object_t *eval_fw(lisp_t * l, object_t * args) {
    return lisp_eval(l, car(args));
}

object_t *assoc_fw(lisp_t * l, object_t * args) {
    return assoc(l, car(args), car(cdr(args)));
}

object_t *pair_fw(lisp_t * l, object_t * args) {
    return pair(l, car(args), car(cdr(args)));
}

#define MAKE_FUNCTION(lisp, name, fptr) do { \
    object_t *f = object_function_new(fptr); \
    object_t *s = object_symbol_new(name); \
    object_t *kv = cons(s, cons(f, NULL)); \
    lisp->env->labels = cons(kv, lisp->env->labels); \
    } while(0);

#define MAKE_BUILTIN(lisp, name, sexpr) do { \
    object_t *obj = lisp_read(lisp, sexpr, strlen(sexpr)); \
    if(NULL == lisp_eval(lisp, obj)) \
        PANIC("lisp_new: could not create %s operator", name); \
    } while(0);

#define SEXPR_DEFUN "(LABEL DEFUN " \
    "(MACRO (NAME ARGS BODY) (LABEL NAME (LAMBDA ARGS BODY))))"

lisp_t *lisp_new() {
    lisp_t *l = calloc(1, sizeof(lisp_t));

    l->env = lisp_env_new(NULL, NULL);
    l->readtable = readtable_new();

    l->t = object_symbol_new("T");

    object_t *nil = object_symbol_new("NIL");

    l->env->labels = cons(cons(nil, NULL), l->env->labels);

    MAKE_FUNCTION(l, "ATOM", atom_fw);
    MAKE_FUNCTION(l, "EQ", eq_fw);
    MAKE_FUNCTION(l, "CAR", car_fw);
    MAKE_FUNCTION(l, "CDR", cdr_fw);
    MAKE_FUNCTION(l, "CONS", cons_fw);
    MAKE_FUNCTION(l, "EVAL", eval_fw);

    MAKE_FUNCTION(l, "PAIR", pair_fw);
    MAKE_FUNCTION(l, "ASSOC", assoc_fw);

    MAKE_BUILTIN(l, "DEFUN", SEXPR_DEFUN);

    return l;
}

object_t *lisp_error(lisp_t * l, object_t * sym) {
    object_t *handpair =
        lisp_env_resolv(l, l->env, object_symbol_new("*ERROR-HANDLER*"));

    object_t *handler = car(cdr(handpair));

    if((handler == NULL) || (handler == NULL))
        PANIC("lisp_error: unhandled error: %s", lisp_print(sym));

    if(handler->type != OBJECT_LAMBDA)
        PANIC("lisp_error: invalid error handler for %s: %s", lisp_print(sym),
              lisp_print(handler));

    object_t *args = ((object_lambda_t *) handler)->args;
    object_t *carg = cons(car(args), cons(sym, NULL));

    l->env->labels = cons(carg, l->env->labels);

    object_t *r = lisp_eval(l, ((object_lambda_t *) handler)->expr);

    ((object_cons_t *) carg)->cdr = NULL;

    return r;
}

/* TODO:
 *   - Split out print_*, read_*
 */
