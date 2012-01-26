#ifndef __LISP_H
#define __LISP_H

#include "list.h"

// TODO - atom is not a type; redo ATOM_* as OBJECT_*
typedef enum {
    ATOM_ERROR,
    ATOM_SYMBOL,
    ATOM_STRING,
    ATOM_INTEGER,
} atom_type_t;

typedef struct {
    atom_type_t type;
} atom_t;

typedef struct {
    atom_t atom;
    const char *name;
} atom_symbol_t;

typedef struct {
    atom_t atom;
    const char *string;
    size_t len;
} atom_string_t;

typedef struct {
    atom_t atom;
    int number;
} atom_integer_t;

typedef enum {
    OBJECT_ERROR,
    OBJECT_ATOM,
    OBJECT_CONS,
    OBJECT_FUNCTION,
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
    atom_t *atom;
} object_atom_t;

typedef struct {
    object_t object;
    object_t *(*fptr) (object_cons_t *);
    int args;
} object_function_t;

typedef struct {
    object_t *object;
} sexpr_t;

sexpr_t *lisp_read(const char *, size_t);
object_t *lisp_eval(sexpr_t *);
const char *lisp_print(object_t *);

#endif
