#ifndef __OBJECT_H
#define __OBJECT_H

#include <stdio.h>

typedef enum object_type_t object_type_t;

typedef struct object_t object_t;
typedef struct object_cons_t object_cons_t;
typedef struct object_function_t object_function_t;
typedef struct object_lambda_t object_lambda_t;
typedef struct object_macro_t object_macro_t;
typedef struct object_integer_t object_integer_t;
typedef struct object_string_t object_string_t;
typedef struct object_symbol_t object_symbol_t;
typedef struct object_stream_t object_stream_t;

enum object_type_t {
    OBJECT_ERROR,
    OBJECT_CONS,
    OBJECT_FUNCTION,
    OBJECT_LAMBDA,
    OBJECT_MACRO,
    OBJECT_INTEGER,
    OBJECT_STRING,
    OBJECT_SYMBOL,
    OBJECT_STREAM,
};

struct object_t {
    object_type_t type;
};

struct object_cons_t {
    object_t object;
    object_t *car;
    object_t *cdr;
};

struct object_lambda_t {
    object_t object;
    object_t *args;
    object_t *expr;
};

struct object_macro_t {
    object_t object;
    object_t *args;
    object_t *expr;
};

struct object_function_t {
    object_t object;
    void *(*fptr) ();
};

struct object_integer_t {
    object_t object;
    int number;
};

struct object_string_t {
    object_t object;
    const char *string;
    size_t len;
};

struct object_symbol_t {
    object_t object;
    const char *name;           // function
    object_t *variable;         // variable
};

struct object_stream_t {
    object_t object;
    FILE *fd;
    int (*read) (object_stream_t *);
    void (*unread) (object_stream_t *, int);
    void (*write) (object_stream_t *, int);
    void (*close) (object_stream_t *);
};

object_t *object_cons_new(object_t *, object_t *);
object_t *object_function_new(void *);
object_t *object_lambda_new(object_t *, object_t *);
object_t *object_macro_new(object_t *, object_t *);
object_t *object_integer_new(int);
object_t *object_string_new(char *, size_t);
object_t *object_symbol_new(char *);
object_t *object_stream_new(FILE *, int (*)(object_stream_t *),
                            void (*)(object_stream_t *, int),
                            void (*)(object_stream_t *, int),
                            void (*)(object_stream_t *));

int object_isa(object_t *, object_type_t);

#endif
