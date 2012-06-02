#ifndef __LISP_PRINT_H
#define __LISP_PRINT_H

#include "lisp.h"
#include "object.h"

object_t *lisp_print(lisp_t *, object_t *);
object_t *lisp_pprint(object_t *);

#endif
