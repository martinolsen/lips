#ifndef __LISP_H
#define __LISP_H

#include "list.h"

typedef struct {
} lisp_t;

typedef enum {
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

typedef struct {
} lisp_list_t;

typedef enum {
    OBJECT_ERROR,
    OBJECT_ATOM,
    OBJECT_LIST,
} object_type_t;

typedef struct {
    object_type_t type;
} object_t;

typedef struct {
    object_t object;
    atom_t *atom;
} object_atom_t;

typedef struct {
    object_t object;
    list_t *list;
} object_list_t;

typedef struct {
    object_t *object;
} sexpr_t;

lisp_t *lisp_new();
void lisp_destroy(lisp_t *);

sexpr_t *lisp_read(lisp_t *, const char *, size_t);
object_t *lisp_eval(lisp_t *, sexpr_t *);
const char *lisp_print(lisp_t *, object_t *);

#endif
