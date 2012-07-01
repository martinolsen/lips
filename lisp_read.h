#ifndef __LISP_READ_H
#define __LISP_READ_H

#include "object.h"
#include "lisp.h"

object_t *lisp_read(lisp_t *, const char *, size_t);
object_t *readtable_new(void);

#endif
