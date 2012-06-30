#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "logger.h"
#include "lisp.h"
#include "lisp_print.h"
#include "builtin.h"
#include "stream.h"

static const char *print_integer(object_t *);
static const char *print_string(object_t *);
static const char *print_cons(object_t *);
static const char *print_object(object_t *);

/** Render object to a string (using lisp_pprint()) and print it, return it. */
object_t *lisp_print(lisp_t * l, object_t * obj) {
    object_t *os_pair =
        lisp_env_resolv(l, l->env, object_symbol_new("*OUTPUT-STREAM*"));

    if(car(cdr(os_pair)) == NULL)
        PANIC("ev_print: *OUTPUT-STREAM* not defined");

    object_t *os = lisp_pprint(obj);

    stream_write_str(car(cdr(os_pair)), os);
    stream_write_char(car(cdr(os_pair)), '\n');

    return obj;
}

/** Render object to object string */
object_t *lisp_pprint(object_t * object) {
    const char *s = print_object(object);

    return object_string_new((char *) s, strlen(s));
}

static const char *print_object(object_t * o) {
    if(o == NULL)
        return "NIL";

    switch (o->type) {
    case OBJECT_ERROR:
        return "ERR";
    case OBJECT_CONS:
        return print_cons(o);
    case OBJECT_INTEGER:
        return print_integer(o);
    case OBJECT_LAMBDA:
        return "#<Lambda>";     // TODO
    case OBJECT_MACRO:
        return "#<Macro>";      // TODO
    case OBJECT_FUNCTION:
        return "#<Function>";   // TODO
    case OBJECT_STRING:
        return print_string(o);
    case OBJECT_SYMBOL:
        return ((object_symbol_t *) o)->name;
    case OBJECT_STREAM:
        PANIC("print_object: cannot print stream");
    }

    PANIC("print_object: unknwon object of type #%d", o->type);

    return NULL;
}

static const char *print_string(object_t * o) {
    if(!object_isa(o, OBJECT_STRING))
        PANIC("print_string: arg is not string!");

    object_string_t *str = (object_string_t *) o;

    const size_t len = str->len + 1;
    char *s = calloc(len, sizeof(char));

    if(snprintf(s, len, "%s", str->string) == 0)
        PANIC("print_string: could not write string %s", str->string);

    return s;
}

static const char *print_integer(object_t * o) {
    if(!object_isa(o, OBJECT_INTEGER))
        PANIC("print_integer: arg is not integer!");

    const size_t len = 15;
    char *s = calloc(len + 1, sizeof(char));

    if(s == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    if(snprintf(s, 10, "%d", ((object_integer_t *) o)->number) == 0) {
        free(s);
        return NULL;
    }

    return s;
}

static const char *print_cons(object_t * list) {
    const size_t len = 1024;
    char *s = calloc(len + 1, sizeof(char));
    size_t si = snprintf(s, len, "(");

    while(list) {
        object_t *obj = list;
        bool dotted = true;

        if(obj->type == OBJECT_CONS)
            dotted = false;

        if(!dotted)
            obj = car(list);

        if((si > 1) && (obj == NULL))
            break;

        const char *prefix = (si > 1) ? (dotted ? " . " : " ") : "";

        si += snprintf(s + si, len - si, "%s%s", prefix, print_object(obj));

        if(si > len)
            PANIC("print_cons[..] - string overflow (got: »%s«)", s);

        if(list->type == OBJECT_CONS)
            list = cdr(list);
        else
            list = NULL;
    }

    snprintf(s + si, len - si, ")");

    return s;
}
