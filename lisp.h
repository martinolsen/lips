#ifndef __LISP_H
#define __LISP_H

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

#include "list.h"

#define TRACE(f, args...) do { \
    /*logger("trace", __FILE__, __LINE__, f, ##args);*/ } while(0);

#define DEBUG(f, args...) do { \
    logger("debug", __FILE__, __LINE__, f, ##args); } while(0);

#define WARN(f, args...) do { \
    logger("warning", __FILE__, __LINE__, f, ##args); } while(0);

#define ERROR(f, args...) do { \
    logger("error", __FILE__, __LINE__, f, ##args); } while(0);

#define PANIC(f, args...) do { \
    logger("panic", __FILE__, __LINE__, f, ##args); \
    BACKTRACE; exit(EXIT_FAILURE); } while(0);

#define BACKTRACE_MAX 100

#define BACKTRACE do { \
    void *buffer[BACKTRACE_MAX]; \
    int j, nptrs = backtrace(buffer, BACKTRACE_MAX); \
    printf("backtrace() returned %d addresses\n", nptrs); \
    char **strings = backtrace_symbols(buffer, nptrs); \
    if(strings == NULL) { perror("backtrace_symbols"); exit(EXIT_FAILURE); } \
    for (j = 0; j < nptrs; j++) printf("%s\n", strings[j]); \
    free(strings); } while(0);

typedef struct lisp_t lisp_t;
typedef struct lisp_env_t lisp_env_t;

typedef struct object_t object_t;
typedef struct object_cons_t object_cons_t;
typedef struct object_function_t object_function_t;
typedef struct object_lambda_t object_lambda_t;
typedef struct object_integer_t object_integer_t;
typedef struct object_string_t object_string_t;
typedef struct object_symbol_t object_symbol_t;

typedef enum object_type_t object_type_t;

lisp_t *lisp_new();

object_t *lisp_read(const char *, size_t);

object_t *cons(object_t *, object_t *);
object_t *car(object_t *);
object_t *cdr(object_t *);
object_t *atom(lisp_t *, object_t *);
object_t *eq(lisp_t *, object_t *, object_t *);
object_t *cond(lisp_t *, lisp_env_t *, object_t *);
object_t *label(lisp_t *, lisp_env_t *, object_t *, object_t *);
object_t *lambda(object_t *, object_t *);

object_t *pair(lisp_t *, object_t *, object_t *);
object_t *assoc(lisp_t *, object_t *, object_t *);

object_t *object_cons_new(object_t *, object_t *);
object_t *object_function_new(void);
object_t *object_lambda_new(object_t *, object_t *);
object_t *object_integer_new(int);
object_t *object_string_new(char *, size_t);
object_t *object_symbol_new(char *);

lisp_env_t *lisp_env_new(lisp_env_t *, object_t *);

void *ALLOC(size_t);

int logger(const char *, const char *, const int, const char *, ...);

enum object_type_t {
    OBJECT_ERROR,
    OBJECT_CONS,
    OBJECT_FUNCTION,
    OBJECT_LAMBDA,
    OBJECT_INTEGER,
    OBJECT_STRING,
    OBJECT_SYMBOL,
};

struct object_t {
    object_type_t type;
};

struct object_cons_t {
    object_t object;
    object_t *car;
    object_t *cdr;
};

struct object_function_t {
    object_t object;
    object_t *(*fptr) (lisp_t *, lisp_env_t *, object_t *);
    int args;
};

struct object_lambda_t {
    object_t object;
    object_t *args;
    object_t *expr;
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
    const char *name;
    // function
    object_t *variable;         // variable
};

struct lisp_env_t {
    object_t *labels;
    lisp_env_t *outer;
};

struct lisp_t {
    lisp_env_t *env;
    object_t *t;
    object_t *nil;
};

#endif
