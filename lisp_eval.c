#include <string.h>

#include "object.h"
#include "logger.h"
#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"
#include "builtin.h"
#include "stream.h"

static object_t *evatom(lisp_t *, object_t *);
static object_t *evfun(lisp_t *, object_t *);
static object_t *evmacr(lisp_t *, object_t *);
static object_t *evlamb(lisp_t *, object_t *);
static object_t *evread(lisp_t *);
static object_t *evloop(lisp_t *, object_t *);
static object_t *evcond(lisp_t *, object_t *);
static object_t *evlis(lisp_t *, object_t *);

/** Evaluate a LISP form.  */
object_t *lisp_eval(lisp_t * l, object_t * exp) {
    if(exp == NULL)
        return NULL;

    if(atom(l, exp))
        return evatom(l, exp);

    if(atom(l, car(exp))) {
        object_t *op = car(exp);

        if(op == NULL)
            PANIC("operator is nil");

        if(eq(l, op, object_symbol_new("QUOTE"))) {
            return car(cdr(exp));
        }
        else if(eq(l, op, object_symbol_new("LAMBDA"))) {
            return lambda(car(cdr(exp)), car(cdr(cdr(exp))));
        }
        else if(eq(l, op, object_symbol_new("MACRO"))) {
            return macro(car(cdr(exp)), car(cdr(cdr(exp))));
        }
        else if(eq(l, op, object_symbol_new("ERROR"))) {
            return lisp_error(l, lisp_eval(l, car(cdr(exp))));
        }
        else if(eq(l, op, object_symbol_new("LABEL"))) {
            return label(l, car(cdr(exp)), lisp_eval(l, car(cdr(cdr(exp)))));
        }
        else if(eq(l, op, object_symbol_new("COND"))) {
            return evcond(l, cdr(exp));
        }
        else if(eq(l, op, object_symbol_new("PRINT"))) {
            return lisp_print(l, lisp_eval(l, car(cdr(exp))));
        }
        else if(eq(l, op, object_symbol_new("LOOP"))) {
            return evloop(l, car(cdr(exp)));
        }
        else if(eq(l, op, object_symbol_new("READ"))) {
            return evread(l);
        }

        switch (op->type) {
        case OBJECT_LAMBDA:
            return evlamb(l, exp);
        case OBJECT_MACRO:
            return evmacr(l, exp);
        case OBJECT_FUNCTION:
            return evfun(l, exp);
        case OBJECT_SYMBOL:
            break;              // will be handled right after switch statement
        case OBJECT_CONS:
        case OBJECT_ERROR:
        case OBJECT_INTEGER:
        case OBJECT_STRING:
        case OBJECT_STREAM:
            PANIC("lisp_eval: something is wrong: %d", exp->type);
        }

        /* op is not one of the builtins, see if it is defined in env */
        // TODO please come up with something smarter than a split environment
        object_t *fpair = lisp_env_resolv(l, l->env, op);

        if(fpair == NULL) {
            ERROR("invalid operator!");

            return NULL;
        }

        return lisp_eval(l, cons(car(cdr(fpair)), cdr(exp)));
    }
    else {
        return lisp_eval(l, cons(lisp_eval(l, car(exp)), cdr(exp)));
    }

    PANIC("lisp_eval: not yet implemented!");
}

static object_t *evatom(lisp_t * l, object_t * exp) {
    switch (exp->type) {
    case OBJECT_INTEGER:
    case OBJECT_STRING:
        return exp;
    case OBJECT_SYMBOL:
        if(eq(l, exp, l->t))
            return l->t;

        object_t *pair = lisp_env_resolv(l, l->env, exp);

        if(pair != NULL)
            return car(cdr(pair));

        return lisp_error(l, object_symbol_new("UNBOUND-SYMBOL"));
    case OBJECT_ERROR:
    case OBJECT_CONS:
    case OBJECT_FUNCTION:
    case OBJECT_LAMBDA:
    case OBJECT_MACRO:
    case OBJECT_STREAM:
        PANIC("lisp_eval: something is wrong: %d", exp->type);
    }

    PANIC("evatom reached the end");

    return NULL;
}

static object_t *evfun(lisp_t * l, object_t * exp) {
    object_function_t *f = (object_function_t *) car(exp);

    // TODO validate args against argdef

    return f->fptr(l, evlis(l, cdr(exp)));
}

static object_t *evmacr(lisp_t * l, object_t * exp) {
    object_macro_t *m = (object_macro_t *) car(exp);

    // TODO validate args against argdef

    /* Pair argument names to input, create env */
    object_t *env_pair = pair(l, m->args, cdr(exp));

    l->env = lisp_env_new(l->env, env_pair);

    object_t *r = lisp_eval(l, m->expr);

    l->env = lisp_env_pop(l->env);

    return r;
}

static object_t *evlamb(lisp_t * l, object_t * exp) {
    object_lambda_t *lamb = (object_lambda_t *) car(exp);

    // TODO validate args agains argdef

    /* Pair argument names to input, create env */
    object_t *args = cdr(exp);
    object_t *env_pair = pair(l, lamb->args, evlis(l, args));

    l->env = lisp_env_new(l->env, env_pair);

    object_t *r = lisp_eval(l, lamb->expr);

    l->env = lisp_env_pop(l->env);

    return r;
}

// TODO mother fsck'er!
static object_t *evread(lisp_t * l) {
    size_t lnsz = 128;
    char ln[lnsz];

    memset(ln, 0, lnsz);
    if(fgets(ln, lnsz - 1, stdin))
        return lisp_read(l, ln, strlen(ln));

    return NULL;
}

static object_t *evloop(lisp_t * l, object_t * obj) {
    while(1)
        lisp_eval(l, car(cdr(obj)));

    PANIC("LOOP should not end");
}

static object_t *evcond(lisp_t * l, object_t * exp) {
    if(exp == NULL)
        return NULL;

    if(lisp_eval(l, car(car(exp))))
        return lisp_eval(l, car(cdr(car(exp))));

    return evcond(l, cdr(exp));
}

static object_t *evlis(lisp_t * l, object_t * exp) {
    if(exp == NULL)
        return NULL;

    return cons(lisp_eval(l, car(exp)), evlis(l, cdr(exp)));
}
