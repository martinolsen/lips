#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>

#include "repl.h"
#include "lisp.h"
#include "lisp_eval.h"

#define BUFSZ 1024
#define REPL_EXPR "(LOOP (PRINT (EVAL (READ))))"

repl_t *repl_init(const char *prompt) {
    repl_t *repl = calloc(1, sizeof(repl_t));

    repl->lisp = lisp_new();
    repl->prompt = prompt;

    return repl;
}

void repl_destroy(repl_t * repl) {
    free(repl);
}

void repl_run(repl_t * repl) {
    object_t *repl_obj = lisp_read(repl->lisp, REPL_EXPR, strlen(REPL_EXPR));

    lisp_eval(repl->lisp, repl_obj);
}
