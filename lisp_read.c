#include <stdlib.h>

#include "lisp_read.h"
#include "lisp.h"
#include "builtin.h"
#include "logger.h"
#include "stream.h"

static object_t *mread_list(lisp_t *, char, object_t *);
static object_t *mread_str(lisp_t *, char, object_t *);
static object_t *mread_quote(lisp_t *, char x, object_t *);
static object_t *mread_unquote(lisp_t *, char, object_t *);
static object_t *mread_backquote(lisp_t *, char, object_t *);

object_t *lisp_read(lisp_t * lisp, const char *s, size_t len) {
    TRACE("lisp_read[_, %s, %d]", s, len);

    object_t *stream = istream_mem(s, len);
    object_t *o = read(lisp, stream);

    return car(macroexpand(lisp, NULL, o));
}

object_t *readtable_new(void) {
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

/** Read list of objects from input-stream. */
static object_t *mread_list(lisp_t * l, char x, object_t * stream) {
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
