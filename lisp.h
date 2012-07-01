#ifndef __LISP_H
#define __LISP_H

#include "list.h"
#include "object.h"

typedef struct lisp_t lisp_t;
typedef struct lisp_env_t lisp_env_t;

struct lisp_env_t {
    object_t *labels;
    lisp_env_t *outer;
};

struct lisp_t {
    lisp_env_t *env;
    object_t *readtable;

    object_t *t;
};

lisp_t *lisp_new();

lisp_env_t *lisp_env_new(lisp_env_t *, object_t *);
lisp_env_t *lisp_env_pop(lisp_env_t *);
object_t *lisp_env_resolv(lisp_t *, lisp_env_t *, object_t *);

object_t *lisp_error(lisp_t *, object_t *);

#endif
