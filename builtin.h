#ifndef __BUILTIN_H
#define __BUILTIN_H

#include "lisp.h"
#include "object.h"

object_t *cons(object_t *, object_t *);
object_t *car(object_t *);
object_t *cdr(object_t *);
object_t *atom(lisp_t *, object_t *);
object_t *eq(lisp_t *, object_t *, object_t *);
object_t *cond(lisp_t *, object_t *);
object_t *label(lisp_t *, object_t *, object_t *);
object_t *lambda(object_t *, object_t *);
object_t *macro(object_t *, object_t *);

object_t *read(lisp_t *, object_t *);

object_t *pair(lisp_t *, object_t *, object_t *);
object_t *assoc(lisp_t *, object_t *, object_t *);
object_t *macroexpand(lisp_t *, object_t *, object_t *);
object_t *macroexpand_1(lisp_t *, object_t *, object_t *);

#endif
