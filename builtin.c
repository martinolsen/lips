#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "builtin.h"
#include "logger.h"
#include "lisp_eval.h"
#include "lisp_print.h"
#include "stream.h"

// Return true if OBJECT is anything other than a CONS
object_t *atom(lisp_t * l, object_t * object) {
    if((object == NULL) || (object->type != OBJECT_CONS))
        return l->t;

    return NULL;
}

/** Construct cons object with car a and cdr b. */
object_t *cons(object_t * a, object_t * b) {
    return object_cons_new(a, b);
}

object_t *car(object_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->type != OBJECT_CONS)
        return NULL;

    return ((object_cons_t *) cons)->car;
}

object_t *cdr(object_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->type != OBJECT_CONS)
        return NULL;

    return ((object_cons_t *) cons)->cdr;
}

object_t *cond(lisp_t * l, object_t * list) {
    if(atom(l, list))
        return NULL;

    object_t *test = car(car(list));
    object_t *res = car(cdr(car(list)));

    if(eq(l, lisp_eval(l, test), l->t))
        return res;

    return cond(l, cdr(list));
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

/** Associate symbol x with value in plist y.
 *
 * Lisp definition:
 *
 *   (defun assoc (x y)
 *     (cond ((eq (caar y) x) (cdar y))
 *             ('t (assoc x (cdr y)))))
 *
 * Example:
 *
 *   (assoc 'b '((a 1) (b 2) (c 3)))
 *   (B 2)
 */
object_t *assoc(lisp_t * l, object_t * x, object_t * o) {
    TRACE("assoc[_, %s, %s]", lisp_print(x), lisp_print(o));

    if(o == NULL)
        return NULL;

    if(o->type != OBJECT_CONS)
        PANIC("assoc: expected list");

    if(atom(l, o))
        return NULL;

    if(eq(l, x, car(car(o))))
        return car(o);

    return assoc(l, x, cdr(o));
}

/** Construct plist from list of keys x and list of values y.
 *
 *  Lisp definition:
 *
 *    (defun pair(x y)
 *      (cond ((and (null x) (null y)) nil)
 *            ((and (not (atom x)) (not (atom y)))
 *              (cons
 *                (list (car x) (car y))
 *                (pair (cdr x) (cdr y))))))
 *
 *  Example:
 *
 *    (pair '(a b c) '(1 2 3))
 *    ((a 1) (b 2) (c 3))
 */
object_t *pair(lisp_t * l, object_t * k, object_t * v) {
    TRACE("pair[%s@%p, %s@%p]", lisp_print(k), k, lisp_print(v), v);

    if((k == NULL) || (v == NULL))
        return NULL;

    if((k->type != OBJECT_CONS) || (k->type != OBJECT_CONS))
        PANIC("pair: expected cons'");

    if(atom(l, k) || atom(l, v))
        return NULL;

    // TODO - make list(object_t *...) helper
    object_t *head = object_cons_new(car(k), object_cons_new(car(v), NULL));
    object_t *tail = pair(l, cdr(k), cdr(v));

    if(car(head) == NULL)
        head = NULL;

    if(car(tail) == NULL)
        tail = NULL;

    return object_cons_new(head, tail);
}

object_t *label(lisp_t * l, object_t * sym, object_t * obj) {
    TRACE("label(_, %s, %s)", lisp_print(sym), lisp_print(obj));

    if(l->env == NULL)
        PANIC("no environment!");

    if(lisp_env_resolv(l, l->env, (object_t *) sym))
        WARN("label: redefining label %s", lisp_print((object_t *) sym));

    object_t *kv = object_cons_new(sym, object_cons_new(obj, NULL));

    l->env->labels = object_cons_new(kv, l->env->labels);

    return obj;
}

/** Construct a lambda object from an s-expression. */
object_t *lambda(object_t * args, object_t * expr) {
    TRACE("lambda[_, %s, %s]", lisp_print((object_t *) args),
          lisp_print(expr));

    return (object_t *) object_lambda_new(args, expr);
}

/** Construct a macro object from an s-expression. */
object_t *macro(object_t * args, object_t * expr) {
    TRACE("macro[_, %s, %s]", lisp_print((object_t *) args),
          lisp_print(expr));

    return (object_t *) object_macro_new(args, expr);
}

/** Read s-expression from stream.
 *
 *  read()'s algorithm is based on that described in
 *  http://www.franz.com/support/documentation/6.2/ansicl/section/readeral.htm
 *
 *  TODO:
 *    * Check if function really is reentrant.
 */
object_t *read(lisp_t * l, object_t * stream) {
    char x;

    do {
        /* 1. If at end of file, end-of-file processing is performed as specified
         * in read.
         */

        // if EOF, exit -- TODO not conforming to above spec
        if(stream_eof(stream))
            return NULL;

        /* 2. If x is an invalid character, an error of type reader-error is
         * signaled.
         */

        // TODO if invalid character, signal read-error

        x = stream_read_char(stream);

        /* 3. If x is a whitespace2 character, then it is discarded and step 1 is
         * re-entered.
         */
    } while((x == ' ') || (x == '\t') || (x == '\n'));

    /* 4. If x is a terminating or non-terminating macro character then its
     * associated reader macro function is called with two arguments, the
     * input stream and x.
     */

    object_t *rt = l->readtable;

    do {
        char x_str[3] = {
            x, 0, 0
        };

        if(!eq(l, car(car(rt)), object_symbol_new(x_str)))
            continue;

        object_t *(*mreader) (lisp_t *, char, object_t *) =
            (void *) cdr(car(rt));

        return (*mreader) (l, x, stream);
    } while((rt = cdr(rt)));

    if(x == ')') {              // TODO - escape characters
        return NULL;
    }

    /* 5. If x is a single escape character then the next character, y, is
     * read, or an error of type end-of-file is signaled if at the end of
     * file.
     */

    // TODO check for escape character

    /* 6. If x is a multiple escape character then a token (initially
     * containing no characters) is begun and step 9 is entered.
     */

    // TODO

    /* 7. If x is a constituent character, then it begins a token. After the
     * token is read in, it will be interpreted either as a Lisp object or as
     * being of invalid syntax.
     */

    // TODO
    static size_t token_sz = 256;
    size_t token_idx = 0;
    char token[token_sz];

    memset(token, 0, token_sz);

    token[token_idx++] = x;

    while(!stream_eof(stream)) {
        if(token_idx > token_sz)
            PANIC("token overflow");

        x = stream_read_char(stream);

        if((x == ' ') || (x == ')') || (x == '\n')) {   // teminating characters
            stream_unread_char(stream, x);
            break;
        }

        token[token_idx++] = x;
    };

    /* 8. At this point a token is being accumulated, and an even number of
     * multiple escape characters have been encountered.
     */

    // TODO

    /* 9. At this point a token is being accumulated, and an odd number of
     * multiple escape characters have been encountered.
     */

    // TODO

    /* 10. An entire token has been accumulated. */

    if(token_idx == 0) {
        WARN("read: token too short: %s", token);
        return NULL;
    }

    // create number object, if possible
    for(size_t i = 0; (token_idx > i) && isdigit(token[i]); i++) {
        if(i == token_idx - 1)
            return object_integer_new(atoi(token));
    }

    return (object_t *) object_symbol_new(token);
}

/** Expand macros in object_t
 *
 * TODO:
 *  * should macro expansion really happen before the reader returns?
 */
object_t *macroexpand(lisp_t * l, object_t * labels, object_t * form) {
    int expanded = 0;

    while(1) {
        object_t *r = macroexpand_1(l, labels, form);

        if(!eq(l, car(cdr(r)), l->t))
            break;

        form = car(r);
        expanded = 1;
    }

    return cons(form, cons(expanded ? l->t : NULL, NULL));
}

static object_t *macroexpand_hook(lisp_t * l, object_t * labels,
                                  object_t * form, void *hook()) {

    int expanded = 0;

    object_t *e = form;
    object_t *exptail = NULL, *exphead = NULL;

    if(e == NULL)
        return cons(form, cons(expanded ? l->t : NULL, NULL));

    do {
        object_t *n = car(e);

        object_t *x = NULL;

        if(atom(l, n)) {
            x = car(cdr(assoc(l, n, labels)));
        }
        else {
            object_t *r = hook(l, labels, n, hook);

            if(!eq(l, cdr(r), l->t)) {
                x = car(r);
                expanded = 1;
            }
        }

        if(x != NULL) {
            n = x;
            expanded = 1;
        }

        if(exphead == NULL)
            exphead = cons(n, NULL);
        else if(exptail == NULL)
            exptail = ((object_cons_t *) exphead)->cdr = cons(n, NULL);
        else
            exptail = ((object_cons_t *) exptail)->cdr = cons(n, NULL);
    } while((e = cdr(e)));

    return cons(exphead, cons(expanded ? l->t : NULL, NULL));
}

object_t *macroexpand_1(lisp_t * l, object_t * labels, object_t * form) {
    int expanded = 0;

    if(form == NULL || atom(l, form) || car(form) == NULL)
        return cons(form, cons(expanded ? l->t : NULL, NULL));

    // do we have a macro form?
    object_t *mpair = lisp_env_resolv(l, l->env, car(form));
    object_t *m = car(cdr(mpair));

    if((mpair == NULL) || (m == NULL) || (m->type != OBJECT_MACRO))
        return cons(form, cons(expanded ? l->t : NULL, NULL));

    // associate the arguments - TODO - old labels are ignored!
    labels = pair(l, ((object_macro_t *) m)->args, cdr(form));

    return macroexpand_hook(l, labels, ((object_macro_t *) m)->expr,
                            (void *(*)()) macroexpand_hook);
}
