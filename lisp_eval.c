#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"

static object_t *eval_object(lisp_t *, lisp_env_t *, object_t *);
static object_t *eval_list(lisp_t *, lisp_env_t *, object_cons_t *);
static object_t *eval_symbol(lisp_t *, lisp_env_t *, object_symbol_t *);

object_t *lisp_eval(lisp_t * l, lisp_env_t * env, object_t * obj) {
    return eval_object(l, env ? env : l->env, obj);
}

static object_t *eval_lambda(lisp_t * l, lisp_env_t * env,
                             object_lambda_t * lm) {

    if(lm == NULL)
        PANIC("eval_lambda: lambda is null");

    DEBUG("eval_lambda[%s, %s]", lisp_print((object_t *) env->labels),
          lisp_print((object_t *) lm));

    object_t *r = lisp_eval(l, env, lm->expr);

    DEBUG(" result: %s", lisp_print(r));

    return r;
}

static object_t *eval_object(lisp_t * l, lisp_env_t * env, object_t * object) {
    if(object == NULL)
        return NULL;

    DEBUG("eval_object[_, _, %s]", lisp_print((object_t *) object));

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

    DEBUG("eval_symbol[_, _, %s]", lisp_print((object_t *) sym));

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

static object_t *eval_list(lisp_t * l, lisp_env_t * env, object_cons_t * list) {
    if((list == NULL) || (car(list) == NULL))
        return NULL;

    DEBUG("eval_list[_, %s, %s]", lisp_print((object_t *) env->labels),
          lisp_print((object_t *) list));

    if(car(list) == NULL)
        PANIC("eval_list: function is NIL");

    object_t *prefix = lisp_eval(l, env, car(list));

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

    DEBUG("eval_list[_, _, %s]", lisp_print((object_t *) list));

    if(prefix->type == OBJECT_LAMBDA) {
        DEBUG("eval_list: LAMBDA!");

        object_lambda_t *lm = (object_lambda_t *) prefix;

        DEBUG(" args: %s, list: %s", lisp_print((object_t *) lm->args),
              lisp_print(cdr(list)));

        object_cons_t *args = (object_cons_t *) cdr(list);
        object_cons_t *labels = pair(l, lm->args, args);

        DEBUG(" 3 labels: %s", lisp_print((object_t *) labels));
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
