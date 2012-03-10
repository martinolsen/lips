#include <stdio.h>
#include <stdlib.h>

#include "repl.h"

int main(void) {
    fprintf(stderr, "       LISP  Interactive  Performance  System\n");

    repl_t *repl = repl_init("LIPS> ");

    repl_run(repl);
    repl_destroy(repl);

    exit(EXIT_SUCCESS);
}
