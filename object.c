#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "object.h"
#include "logger.h"

static void *ALLOC(const size_t sz) {
    void *o = calloc(1, sz);

    if(o == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    return o;
}

static size_t object_sz(object_type_t type) {
    switch (type) {
    case OBJECT_CONS:
        return sizeof(object_cons_t);
    case OBJECT_FUNCTION:
        return sizeof(object_function_t);
    case OBJECT_LAMBDA:
        return sizeof(object_lambda_t);
    case OBJECT_MACRO:
        return sizeof(object_macro_t);
    case OBJECT_INTEGER:
        return sizeof(object_integer_t);
    case OBJECT_STRING:
        return sizeof(object_string_t);
    case OBJECT_SYMBOL:
        return sizeof(object_symbol_t);
    case OBJECT_STREAM:
        return sizeof(object_stream_t);
    case OBJECT_ERROR:
        PANIC("object_new: unknwon error");
    }

    ERROR("object_new: unknown type");
    exit(EXIT_FAILURE);
}

static object_t *object_new(object_type_t type) {
    object_t *object = ALLOC(object_sz(type));

    object->type = type;

    return object;
}

object_t *object_cons_new(object_t * car, object_t * cdr) {
    object_cons_t *cons = (object_cons_t *) object_new(OBJECT_CONS);

    cons->car = car;
    cons->cdr = cdr;

    return (object_t *) cons;
}

object_t *object_function_new(void *fptr) {
    object_function_t *of = (object_function_t *) object_new(OBJECT_FUNCTION);

    of->fptr = fptr;

    return (object_t *) of;
}

object_t *object_lambda_new(object_t * args, object_t * expr) {

    object_lambda_t *l = (object_lambda_t *) object_new(OBJECT_LAMBDA);

    l->args = args;
    l->expr = expr;

    return (object_t *) l;
}

object_t *object_macro_new(object_t * args, object_t * expr) {

    object_macro_t *l = (object_macro_t *) object_new(OBJECT_MACRO);

    l->args = args;
    l->expr = expr;

    return (object_t *) l;
}

object_t *object_integer_new(int num) {
    object_integer_t *o = (object_integer_t *) object_new(OBJECT_INTEGER);

    o->number = num;
    return (object_t *) o;
}

object_t *object_string_new(char *s, size_t n) {
    object_string_t *o = (object_string_t *) object_new(OBJECT_STRING);

    o->string = s;
    o->len = n;

    return (object_t *) o;
}

object_t *object_symbol_new(char *s) {
    object_symbol_t *o = (object_symbol_t *) object_new(OBJECT_SYMBOL);
    size_t sz = strlen(s);

    o->name = ALLOC(sz);

    memcpy((char *) o->name, s, sz);

    return (object_t *) o;
}

object_t *object_stream_new(const char *buf, size_t sz) {
    object_stream_t *o = (object_stream_t *) object_new(OBJECT_STREAM);

    o->buf = ALLOC(sz);
    o->buf_sz = sz;
    o->buf_idx = 0;

    memcpy(o->buf, buf, sz);

    return (object_t *) o;
}

int object_isa(object_t * o, object_type_t t) {
    if(o == NULL)
        return 0;

    if(o->type != t)
        return 0;

    return 1;
}
