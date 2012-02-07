#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"

static object_t *eval_object(lisp_t *, lisp_env_t *, object_t *);
static object_t *eval_list(lisp_t *, lisp_env_t *, object_t *);
static object_t *eval_symbol(lisp_t *, lisp_env_t *, object_t *);

/*
01: (defun eval (e a)
02:   (cond
03:     ((atom e) (assoc e a))
04:     ((atom (car e))
05:      (cond
06:        ((eq (car e) 'quote) (cadr e))
07:        ((eq (car e) 'atom)  (atom (eval (cadr e) a)))
08:        ((eq (car e) 'eq)    (eq   (eval (cadr e) a)
09:                                   (eval (caddr e) a)))
10:        ((eq (car e) 'car)   (car  (eval (cadr e) a)))
11:        ((eq (car e) 'cdr)   (cdr  (eval (cadr e) a)))
12:        ((eq (car e) 'cons)  (cons (eval (cadr e) a)
13:                                   (eval (caddr e) a)))
14:        ((eq (car e) 'cond)  (evcon (cdr e) a))
15:        ('t (eval (cons (assoc (car e) a)
16:                        (cdr e))
17:                  a))))
18:     ((eq (caar e) 'label)
19:      (eval (cons (caddar e) (cdr e))
20:            (cons (list (cadar e) (car e)) a)))
21:     ((eq (caar e) 'lambda)
22:      (eval (caddar e)
23:            (append (pair (cadar e) (evlis (cdr e) a))
24:                    a)))))
30:
31: (defun evcon (c a)
32:   (cond ((eval (caar c) a)
33:          (eval (cadar c) a))
34:         ('t (evcon (cdr c) a))))
40:
41: (defun evlis (m a)
42:   (cond ((null m) '())
43:         ('t (cons (eval (car m) a)
44:                   (evlis (cdr m) a)))))
*/
object_t *lisp_eval(lisp_t * l, lisp_env_t * env, object_t * exp) {
    TRACE("lisp_eval(_, _, %s)", lisp_print(exp));

    if(atom(l, exp)) {
        // 03: ((atom e) (assoc e a))

        if(eq(l, exp, l->t))
            return l->t;

        return assoc(l, exp,
                     (object_t *) (env ? env->labels : l->env->labels));
    }
    else if(atom(l, car(exp))) {
        // 04: ((atom (car e))
    }
    else if(eq(l, car(car(exp)), lisp_read("(QUOTE LABEL)", 14))) {
        // 18: ((eq (caar e) 'label)
    }
    else if( /* TODO */ 0) {
        // 21: ((eq (caar e) 'lambda)
    }
    else {
        PANIC("cannot eval expression: %s\n", lisp_print(exp));
    }

    return eval_object(NULL, NULL, NULL);   // XXX
}

static object_t *eval_lambda(lisp_t * l, lisp_env_t * env, object_t * o) {

    TRACE("eval_lambda[%s, %s]", lisp_print((object_t *) env->labels),
          lisp_print(o));

    if((o == NULL) || (o->type != OBJECT_LAMBDA))
        PANIC("eval_lambda: expected lambda");

    object_t *r = lisp_eval(l, env, ((object_lambda_t *) o)->expr);

    TRACE(" result: %s", lisp_print(r));

    return r;
}

static object_t *eval_object(lisp_t * l, lisp_env_t * env, object_t * object) {
    TRACE("eval_object[_, _, %s]", lisp_print(object));

    if(object == NULL)
        return NULL;

    switch (object->type) {
    case OBJECT_CONS:
        return eval_list(l, env, object);
    case OBJECT_SYMBOL:
        return eval_symbol(l, env, object);
    case OBJECT_STRING:
    case OBJECT_INTEGER:
        return object;
    case OBJECT_LAMBDA:
        return eval_lambda(l, env, object);
    case OBJECT_FUNCTION:
        PANIC("Trying to evaluate function!");
    case OBJECT_ERROR:
        ERROR("eval_object: unexpected error");
    }

    ERROR("eval_object: unhandled object %s", lisp_print(object));
    exit(EXIT_FAILURE);
}

static object_t *eval_symbol(lisp_t * l, lisp_env_t * env, object_t * sym) {

    TRACE("eval_symbol[_, _, %s]", lisp_print(sym));

    if(sym->type != OBJECT_SYMBOL)
        PANIC("eval_symbol: expected symbol!");

    if(eq(l, sym, object_symbol_new("NIL")))
        return NULL;

    if(eq(l, sym, l->t))
        return l->t;

    while(env != NULL) {
        object_t *obj = assoc(l, sym, (object_t *) env->labels);

        if(obj != NULL)
            return obj;

        env = env->outer;
    }

    PANIC("eval_symbol[_, %s] - no such symbol in environment",
          lisp_print((object_t *) sym));

    return NULL;
}

static object_t *eval_list(lisp_t * l, lisp_env_t * env, object_t * list) {
    TRACE("eval_list[_, %s, %s]", lisp_print((object_t *) env->labels),
          lisp_print(list));

    if((list == NULL) || (car(list) == NULL))
        return NULL;

    if(car(list) == NULL)
        PANIC("eval_list: function is NIL");

    object_t *prefix = lisp_eval(l, env, car(list));

    if(!atom(l, prefix)) {
        PANIC("eval_list: prefix must be atom, not %s, in %s",
              lisp_print(prefix), lisp_print(list));
    }

    if(prefix == NULL)
        PANIC("eval_list[_, %s] - prefix is NIL", lisp_print(list));

    if(prefix->type == OBJECT_FUNCTION) {
        object_function_t *fun = (object_function_t *) prefix;

        if((fun->args != -1) && (fun->args != (list_length(list) - 1))) {
            PANIC("function %s takes %d arguments",
                  lisp_print(prefix), fun->args);
        }

        // TODO remove args, should be in env - like with lambdas
        return (*fun->fptr) (l, env, cdr(list));
    }

    if(prefix->type == OBJECT_LAMBDA) {
        TRACE("eval_list: LAMBDA!");

        object_lambda_t *lm = (object_lambda_t *) prefix;

        TRACE(" args: %s, list: %s", lisp_print((object_t *) lm->args),
              lisp_print(cdr(list)));

        object_t *args = cdr(list);
        object_t *labels = pair(l, (object_t *) lm->args, args);

        TRACE(" 3 labels: %s", lisp_print(labels));
        lisp_env_t *e = lisp_env_new(env, labels);

        return eval_lambda(l, e, (object_t *) lm);
    }

    PANIC("ARE WE DONE HERE?");
    (void) pair(l, NULL, NULL);

    if(prefix->type != OBJECT_SYMBOL)
        PANIC("eval_list: expecting symbol, not %s", lisp_print(prefix));

    object_symbol_t *fun_symbol = (object_symbol_t *) prefix;

    PANIC("TODO, next line!");
    //object_function_t *fun = (object_function_t *) assoc(l, prefix, l->env);
    object_function_t *fun = NULL;

    if(fun->object.type != OBJECT_FUNCTION) {
        PANIC("eval_list: unknown function name %s in %s",
              fun_symbol->name, lisp_print(list));
    }

    if((fun->args != -1) && (fun->args != (list_length(list) - 1))) {
        PANIC("function %s takes %d arguments",
              lisp_print(prefix), fun->args);
    }

    // TODO remove args, should be in env
    object_t *r = (*fun->fptr) (l, lisp_env_new(env, NULL), cdr(list));

    return r;
}
