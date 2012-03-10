#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>

#include "repl.h"
#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"

#define BUFSZ 1024

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
    int running = 1;

    while(running) {
        char *src = readline(repl->prompt);

        if(src == NULL)
            continue;

        size_t len = strlen(src);

        if(len == 0)
            continue;

        object_t *robj = lisp_read(src, len);
        object_t *eobj = lisp_eval(repl->lisp, NULL, robj);

        printf("res: %s\n", lisp_print(eobj));

        free(src);
    }
}
