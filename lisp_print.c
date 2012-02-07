#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "lisp.h"
#include "lisp_print.h"

static const char *print_integer(object_t *);
static const char *print_string(object_t *);
static const char *print_cons(object_t *);
static const char *print_object(object_t *);

// Render object to string. Client must free() returned value.
const char *lisp_print(object_t * object) {
    return print_object(object);
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
    case OBJECT_FUNCTION:
        return "#<Function>";   // TODO
    case OBJECT_STRING:
        return print_string(o);
    case OBJECT_SYMBOL:
        return ((object_symbol_t *) o)->name;
    }

    PANIC("print_object: unknwon object of type #%d", o->type);

    return NULL;
}

#define ASSERT_ISA(o, t, m) if((o == NULL) || (o->type != t)) PANIC(m);

static const char *print_string(object_t * o) {
    ASSERT_ISA(o, OBJECT_STRING, "print_string: arg is not string!");

    object_string_t *str = (object_string_t *) o;

    const size_t len = str->len + 3;
    char *s = calloc(len, sizeof(char));

    if(snprintf(s, len, "\"%s\"", str->string) == 0)
        PANIC("print_string: could not write string %s", str->string);

    return s;
}

static const char *print_integer(object_t * o) {
    ASSERT_ISA(o, OBJECT_INTEGER, "print_integer: arg is not integer!");

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
    const size_t len = 255;
    char *s = ALLOC(len * sizeof(char) + 1);
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
            PANIC("print_cons[..] - string overflow");

        if(list->type == OBJECT_CONS)
            list = cdr(list);
        else
            list = NULL;
    }

    snprintf(s + si, len - si, ")");

    return s;
}
