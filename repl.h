#ifndef __REPL_H
#define __REPL_H

#include "lisp.h"

typedef struct {
    lisp_t *lisp;
    const char *prompt;
} repl_t;

repl_t *repl_init(const char *);
void repl_destroy(repl_t *);
void repl_run(repl_t *);

#endif
