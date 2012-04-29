#include <stdio.h>
#include <stdlib.h>

#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"

int main(void) {
    FILE *repl_file = fopen("repl.lips", "r");

    if(repl_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fseek(repl_file, 0L, SEEK_END);
    size_t bufsz = ftell(repl_file);

    fseek(repl_file, 0L, SEEK_SET);

    char *buf = (char *) calloc(bufsz + 1, sizeof(char));

    if(buf == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    fread(buf, sizeof(char), bufsz, repl_file);
    fclose(repl_file);

    lisp_t *lisp = lisp_new();

    size_t i = 0;
    char pprev = 0, prev = 0;

    while(i < bufsz) {
        if(prev == 0 || (pprev == '\n' && prev == '\n' && buf[i] != '\n'))
            lisp_eval(lisp, lisp_read(lisp, buf + i, bufsz - i));

        pprev = prev;
        prev = buf[i++];
    }

    free(buf);

    exit(EXIT_SUCCESS);
}
