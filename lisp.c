#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "logger.h"
#include "lisp.h"
#include "lisp_print.h"
#include "lisp_eval.h"
#include "lexer.h"
#include "list.h"
#include "object.h"

static object_t *macroexpand(lisp_t *, object_t *, object_t *);
static object_t *macroexpand_1(lisp_t *, object_t *, object_t *);
static object_t *macroexpand_hook(lisp_t *, object_t *, object_t *,
                                  void *(*)());

object_t *lisp_read(lisp_t * lisp, const char *s, size_t len) {
    TRACE("lisp_read[_, %s, %d]", s, len);

    object_t *stream = object_stream_new(s, len);
    object_t *o = read(lisp, stream);

    return car(macroexpand(lisp, NULL, o));
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

/** Construct cons object with car a and cdr b. */
object_t *cons(object_t * a, object_t * b) {
    return object_cons_new(a, b);
}

object_t *car(object_t * cons) {
    if(cons == NULL)
        return NULL;

    if(cons->type != OBJECT_CONS) {
        PANIC("car: object is not a list: »%s« (%d)\n",
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

/** Associate symbol x with value in plist y.
 *
 * Lisp definition:
 *
 *   (defun assoc (x y)
 *     (cond ((eq (caar y) x) (cadar y))
 *             ('t (assoc x (cdr y)))))
 *
 * Example:
 *
 *   (assoc 'b '((a 1) (b 2) (c 3)))
 *   2
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
        return car(cdr(car(o)));

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
// TODO add test
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

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// STREAMS & READERS /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

static int stream_is_eof(object_t * o) {
    if(!object_isa(o, OBJECT_STREAM))
        return 1;

    object_stream_t *s = (object_stream_t *) o;

    if(s->buf_idx >= s->buf_sz)
        return 1;

    return 0;
}

static char stream_read_char(object_t * o) {
    if(!object_isa(o, OBJECT_STREAM))
        return EOF;

    if(stream_is_eof(o))
        return EOF;

    object_stream_t *s = (object_stream_t *) o;

    return s->buf[s->buf_idx++];
}

static void stream_unread_char(object_t * o, char x) {
    if(!object_isa(o, OBJECT_STREAM))
        PANIC("cannot unread char to non-stream object");

    object_stream_t *s = (object_stream_t *) o;

    if(s->buf_idx == 0)
        PANIC("unreading beyond beginning of stream not supported");

    s->buf[--s->buf_idx] = x;
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

    while(!stream_is_eof(stream)) {
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
        if(stream_is_eof(stream))
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

    while(!stream_is_eof(stream)) {
        if(token_idx > token_sz)
            PANIC("token overflow");

        x = stream_read_char(stream);

        if((x == ' ') || (x == ')')) {  // teminating characters
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

object_t *atom_fw(lisp_t * l, lisp_env_t * env, object_t * args) {
    env = env;

    return atom(l, car(args));
}

object_t *eq_fw(lisp_t * l, lisp_env_t * env, object_t * args) {
    env = env;

    return eq(l, car(args), car(cdr(args)));
}

object_t *car_fw(lisp_t * l, lisp_env_t * env, object_t * args) {
    l = l;
    env = env;

    return car(car(args));
}

object_t *cdr_fw(lisp_t * l, lisp_env_t * env, object_t * args) {
    l = l;
    env = env;

    return cdr(car(args));
}

object_t *cons_fw(lisp_t * l, lisp_env_t * env, object_t * args) {
    l = l;
    env = env;

    return cons(car(args), car(cdr(args)));
}

object_t *eval_fw(lisp_t * l, lisp_env_t * env, object_t * args) {
    return lisp_eval(l, env, car(args));
}

object_t *assoc_fw(lisp_t * l, lisp_env_t * env, object_t * args) {
    env = env;

    return assoc(l, car(args), car(cdr(args)));
}

object_t *pair_fw(lisp_t * l, lisp_env_t * env, object_t * args) {
    env = env;

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
    if(NULL == lisp_eval(lisp, lisp->env, obj)) \
        PANIC("lisp_new: could not create %s operator", name); \
    } while(0);

#define SEXPR_DEFUN "(LABEL DEFUN " \
    "(MACRO (NAME ARGS BODY) (LABEL NAME (LAMBDA ARGS BODY))))"

lisp_t *lisp_new() {
    lisp_t *l = calloc(1, sizeof(lisp_t));

    l->env = lisp_env_new(NULL, NULL);
    l->readtable = readtable_new();

    l->t = object_symbol_new("T");
    l->nil = object_symbol_new("NIL");

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

/** Expand macros in object_t
 *
 * TODO:
 *  * should macro expansion really happen before the reader returns?
 */
static object_t *macroexpand(lisp_t * l, object_t * labels, object_t * form) {
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

static object_t *macroexpand_1(lisp_t * l, object_t * labels, object_t * form) {
    int expanded = 0;

    if(form == NULL || atom(l, form) || car(form) == NULL)
        return cons(form, cons(expanded ? l->t : NULL, NULL));

    // do we have a macro form?
    object_t *m = assoc(l, car(form), l->env->labels);

    if(m == NULL || m->type != OBJECT_MACRO)
        return cons(form, cons(expanded ? l->t : NULL, NULL));

    // associate the arguments - TODO - old labels are ignored!
    labels = pair(l, ((object_macro_t *) m)->args, cdr(form));

    return macroexpand_hook(l, labels, ((object_macro_t *) m)->expr,
                            (void *(*)()) macroexpand_hook);
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
            x = assoc(l, n, labels);
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

/* TODO:
 *   - Split out print_*, read_*
 */
