#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"

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

            return assoc(l, exp, env ? env->labels : l->env->labels);
        case OBJECT_ERROR:
        case OBJECT_CONS:
        case OBJECT_FUNCTION:
        case OBJECT_LAMBDA:
            PANIC("lisp_eval: something is wrong: %d", exp->type);
        }
    }
    else if(atom(l, car(exp))) {
        // 04: ((atom (car e))
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

        return lisp_eval(l, env,
                         cons(assoc(l, car(exp), env ? env->labels : NULL),
                              cdr(exp)));
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
