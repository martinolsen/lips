#include <string.h>

#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"

static object_t *evread(void);
static object_t *evprint(object_t *);
static object_t *evloop(lisp_t *, lisp_env_t *, object_t *);
static object_t *evcond(lisp_t *, lisp_env_t *, object_t *);
static object_t *evlis(lisp_t *, lisp_env_t *, object_t *);

object_t *lisp_eval(lisp_t * l, lisp_env_t * env, object_t * exp) {
    TRACE("lisp_eval(_, _, %s)", lisp_print(exp));

    if(exp == NULL)
        return NULL;

    if(atom(l, exp)) {
        switch (exp->type) {
        case OBJECT_INTEGER:
        case OBJECT_STRING:
            return exp;
        case OBJECT_SYMBOL:
            if(eq(l, exp, l->t))
                return l->t;

            if(eq(l, exp, l->nil))
                return NULL;

            object_t *obj = assoc(l, exp, env ? env->labels : l->env->labels);

            if(obj != NULL)
                return obj;

            PANIC("undefined symbol: %s", exp);
        case OBJECT_ERROR:
        case OBJECT_CONS:
        case OBJECT_FUNCTION:
        case OBJECT_LAMBDA:
            PANIC("lisp_eval: something is wrong: %d", exp->type);
        }
    }
    else if(atom(l, car(exp))) {
        if(eq(l, car(exp), object_symbol_new("QUOTE"))) {
            return car(cdr(exp));
        }
        else if(eq(l, car(exp), object_symbol_new("ATOM"))) {
            return atom(l, lisp_eval(l, env, car(cdr(exp))));
        }
        else if(eq(l, car(exp), object_symbol_new("EQ"))) {
            return eq(l, lisp_eval(l, env, car(cdr(exp))),
                      lisp_eval(l, env, car(cdr(cdr(exp)))));
        }
        else if(eq(l, car(exp), object_symbol_new("CAR"))) {
            return car(lisp_eval(l, env, car(cdr(exp))));
        }
        else if(eq(l, car(exp), object_symbol_new("CDR"))) {
            return cdr(lisp_eval(l, env, car(cdr(exp))));
        }
        else if(eq(l, car(exp), object_symbol_new("CONS"))) {
            return cons(lisp_eval(l, env, car(cdr(exp))),
                        lisp_eval(l, env, car(cdr(cdr(exp)))));
        }
        else if(eq(l, car(exp), object_symbol_new("COND"))) {
            return evcond(l, env, cdr(exp));
        }
        else if(eq(l, car(exp), object_symbol_new("LABEL"))) {
            return label(l, env, car(cdr(exp)), car(cdr(cdr(exp))));
        }
        else if(eq(l, car(exp), object_symbol_new("EVAL"))) {
            return lisp_eval(l, env, lisp_eval(l, env, car(cdr(exp))));
        }
        else if(eq(l, car(exp), object_symbol_new("PRINT"))) {
            return evprint(car(cdr(exp)));
        }
        else if(eq(l, car(exp), object_symbol_new("LOOP"))) {
            return evloop(l, env, car(cdr(exp)));
        }
        else if(eq(l, car(exp), object_symbol_new("READ"))) {
            return evread();
        }

        object_t *operator = assoc(l, car(exp), env ? env->labels : NULL);

        if(operator == NULL) {
            ERROR("invalid operator %s", lisp_print(car(exp)));
            return NULL;
        }

        object_t *tail = cdr(exp);

        return lisp_eval(l, env, cons(operator, tail));
    }
    else if(eq(l, car(car(exp)), object_symbol_new("LAMBDA"))) {
        return lisp_eval(l,
                         lisp_env_new(env, pair(l,
                                                car(cdr(car(exp))),
                                                evlis(l, env, cdr(exp)))),
                         car(cdr(cdr(car(exp)))));
    }

    PANIC("lisp_eval: not yet implemented: %s", lisp_print(exp));
}

// TODO mother fsck'er!
static object_t *evread(void) {
    size_t lnsz = 128;
    char ln[lnsz];

    memset(ln, 0, lnsz);
    if(fgets(ln, lnsz - 1, stdin))
        return lisp_read(ln, strlen(ln));

    return NULL;
}

static object_t *evprint(object_t * obj) {
    lisp_print(obj);

    return obj;
}

static object_t *evloop(lisp_t * l, lisp_env_t * env, object_t * obj) {
    while(1)
        lisp_eval(l, env, car(cdr(obj)));

    PANIC("LOOP should not end");
}

static object_t *evcond(lisp_t * l, lisp_env_t * env, object_t * exp) {
    TRACE("evcond[_, _, %s]", lisp_print(exp));

    if(exp == NULL)
        return NULL;

    if(lisp_eval(l, env, car(car(exp))))
        return lisp_eval(l, env, car(cdr(car(exp))));

    return evcond(l, env, cdr(exp));
}

static object_t *evlis(lisp_t * l, lisp_env_t * env, object_t * exp) {
    TRACE("evlis[_, _, %s]", lisp_print(exp));

    if(exp == NULL)
        return NULL;

    return cons(lisp_eval(l, env, car(exp)), evlis(l, env, cdr(exp)));
}
