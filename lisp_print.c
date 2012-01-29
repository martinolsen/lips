#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "lisp.h"
#include "lisp_print.h"

static const char *print_integer(object_integer_t *);
static const char *print_string(object_string_t *);
static const char *print_cons(object_cons_t *);
static const char *print_object(object_t *);

// Render object to string. Client must free() returned value.
const char *lisp_print(object_t * object) {
    return print_object(object);
}

static const char *print_object(object_t * o) {
    if(o == NULL)
        return "NIL";

    DEBUG("print_object[_] - type %d", o->type);

    switch (o->type) {
    case OBJECT_ERROR:
        return "ERR";
    case OBJECT_CONS:
        return print_cons((object_cons_t *) o);
    case OBJECT_INTEGER:
        return print_integer((object_integer_t *) o);
    case OBJECT_LAMBDA:
        return "#<Lambda>";     // TODO
    case OBJECT_FUNCTION:
        return "#<Function>";   // TODO
    case OBJECT_STRING:
        return print_string((object_string_t *) o);
    case OBJECT_SYMBOL:
        return ((object_symbol_t *) o)->name;
    }

    ERROR("print_object: unknwon object of type #%d", o->type);

    return NULL;
}

static const char *print_string(object_string_t * str) {
    const size_t len = str->len + 3;
    char *s = calloc(len, sizeof(char));

    if(snprintf(s, len, "\"%s\"", str->string) == 0)
        PANIC("print_string: could not write string %s", str->string);

    return s;
}

static const char *print_integer(object_integer_t * integer) {
    const size_t len = 15;
    char *s = calloc(len + 1, sizeof(char));

    if(s == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    if(snprintf(s, 10, "%d", integer->number) == 0) {
        free(s);
        return NULL;
    }

    return s;
}

static const char *print_cons(object_cons_t * list) {
    const size_t len = 255;
    char *s = ALLOC(len * sizeof(char) + 1);
    size_t si = snprintf(s, len, "(");

    while(list) {
        object_t *obj = (object_t *) list;
        bool dotted = true;

        DEBUG("a");

        if(obj->type == OBJECT_CONS)
            dotted = false;

        if(!dotted)
            obj = car(list);

        DEBUG("b");

        if((si > 1) && (obj == NULL))
            break;

        DEBUG("c");

        const char *prefix = (si > 1) ? (dotted ? " . " : " ") : "";

        DEBUG("d");

        si += snprintf(s + si, len - si, "%s%s", prefix, print_object(obj));

        DEBUG("e");

        if(si > len)
            PANIC("print_cons[..] - string overflow");

        DEBUG("f");

        if(list->object.type == OBJECT_CONS)
            list = (object_cons_t *) cdr(list);
        else
            list = NULL;

        DEBUG("g");
    }

    /*
     * size_t si = snprintf(0, len, ".");
     * 
     * 
     * do {
     * object_t *obj = car(list);
     * 
     * 
     * } while(obj != NULL);
     * 
     * while(0) {
     * if(si > len) {
     * ERROR("si > len");
     * free(s);
     * return NULL;
     * }
     * 
     * object_t *element = (object_t *) list;
     * bool dotted = true;
     * 
     * if(list->object.type == OBJECT_CONS) {
     * element = car(list);
     * dotted = false;
     * }
     * 
     * char *restrict start = s + si;
     * const size_t end = len - si;
     * 
     * const char *prefix = si ? (dotted ? " . " : " ") : "(";
     * 
     * si += snprintf(start, end, "%s%s", prefix, print_object(element));
     * 
     * // TODO - bounds check
     * 
     * if(dotted)
     * break;
     * 
     * list = (object_cons_t *) cdr(list);
     * };
     * 
     * if((list != NULL) && (list->object.type != OBJECT_CONS)) {
     * char *restrict start = s + si;
     * const size_t end = len - si;
     * 
     * si += snprintf(start, end, " . %s", print_object((object_t *) list));
     * }
     */

    snprintf(s + si, len - si, ")");

    DEBUG("print_cons[..]: %s", s);

    return s;
}
