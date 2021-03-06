#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "logger.h"
#include "lisp.h"
#include "lisp_eval.h"
#include "object.h"
#include "stream.h"
#include "builtin.h"
#include "lisp_print.h"
#include "lisp_read.h"

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

    object_t *pairi = env->labels;

    while(pairi) {
        if(eq(l, car(car(pairi)), x))
            return car(pairi);

        pairi = cdr(pairi);
    }

    return lisp_env_resolv(l, env->outer, x);
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

static object_t *format(lisp_t *l __attribute__ ((unused)), object_t *fmt,
        object_t *args) {

    if(fmt == NULL)
        return NULL;

    if(!object_isa(fmt, OBJECT_STRING))
        PANIC("format: fmt is not a string!");

    object_string_t *fmts = (object_string_t *)fmt;

    size_t fmtsi = 0, outi = 0, outsz = 1024;
    char *out = calloc(outsz, sizeof(char));

    if(out == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    while(fmtsi < fmts->len && fmts->string[fmtsi]) {
        char c = fmts->string[fmtsi++];

        if(c != '~') {
            out[outi++] = c;
            continue;
        }

        object_t *o = NULL;

        switch(c = fmts->string[fmtsi++]) {
            case 'a':
                o = lisp_pprint(car(args));
                break;
            default:
                PANIC("format: unknown control char");
        }

        if(o == NULL)
            PANIC("format: o is nil");

        if(!object_isa(o, OBJECT_STRING))
            PANIC("format: expected string");

        object_string_t *os = (object_string_t *) o;

        if(os->len + outi > outsz)
            PANIC("format: element too large");

        strcpy(out + outi, os->string);

        outi += os->len;
        args = cdr(args);
    }

    return object_string_new(out, outi);
}

object_t *format_fw(lisp_t * l, object_t * args) {
    return format(l, car(args), cdr(args));
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

    object_t *os_pair = cons(object_symbol_new("*OUTPUT-STREAM*"),
                             cons(ostream_file("/dev/stdout"), NULL));

    l->env->labels = cons(os_pair, l->env->labels);

    MAKE_FUNCTION(l, "ATOM", atom_fw);
    MAKE_FUNCTION(l, "EQ", eq_fw);
    MAKE_FUNCTION(l, "CAR", car_fw);
    MAKE_FUNCTION(l, "CDR", cdr_fw);
    MAKE_FUNCTION(l, "CONS", cons_fw);
    MAKE_FUNCTION(l, "EVAL", eval_fw);

    MAKE_FUNCTION(l, "PAIR", pair_fw);
    MAKE_FUNCTION(l, "ASSOC", assoc_fw);
    MAKE_FUNCTION(l, "FORMAT", format_fw);

    MAKE_BUILTIN(l, "DEFUN", SEXPR_DEFUN);

    return l;
}

object_t *lisp_error(lisp_t * l, object_t * sym) {
    object_t *handpair =
        lisp_env_resolv(l, l->env, object_symbol_new("*ERROR-HANDLER*"));

    object_t *handler = car(cdr(handpair));

    if((handler == NULL) || (handler == NULL))
        PANIC("lisp_error: unhandled error!");

    if(handler->type != OBJECT_LAMBDA)
        PANIC("lisp_error: invalid error handler!");

    object_t *args = ((object_lambda_t *) handler)->args;
    object_t *carg = cons(car(args), cons(sym, NULL));

    l->env->labels = cons(carg, l->env->labels);

    object_t *r = lisp_eval(l, ((object_lambda_t *) handler)->expr);

    ((object_cons_t *) carg)->cdr = NULL;

    return r;
}
