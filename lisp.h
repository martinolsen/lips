#ifndef __LISP_H
#define __LISP_H

#include "list.h"

typedef enum {
    OBJECT_ERROR,
    OBJECT_CONS,
    OBJECT_FUNCTION,
    OBJECT_INTEGER,
    OBJECT_STRING,
    OBJECT_SYMBOL,
} object_type_t;

typedef struct {
    object_type_t type;
} object_t;

typedef struct {
    object_t object;
    object_t *car;
    object_t *cdr;
} object_cons_t;

typedef struct {
    object_t object;
    object_t *(*fptr) (object_cons_t *);
    int args;
} object_function_t;

typedef struct {
    object_t object;
    int number;
} object_integer_t;

typedef struct {
    object_t object;
    const char *string;
    size_t len;
} object_string_t;

typedef struct {
    object_t object;
    const char *name;
} object_symbol_t;

typedef struct {
    object_t *object;
} sexpr_t;

sexpr_t *lisp_read(const char *, size_t);
object_t *lisp_eval(sexpr_t *);
const char *lisp_print(object_t *);

#endif
