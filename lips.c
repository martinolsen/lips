#include <stdio.h>
#include <stdlib.h>

#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_read.h"

void eval_file(lisp_t *lisp, char *filename) {
    FILE *repl_file = fopen(filename, "r");

    fprintf(stderr, "%s ...\n", filename);

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

    size_t i = 0;
    char pprev = 0, prev = 0;

    while(i < bufsz) {
        if(prev == 0 || (pprev == '\n' && prev == '\n' && buf[i] != '\n'))
            lisp_eval(lisp, lisp_read(lisp, buf + i, bufsz - i));

        pprev = prev;
        prev = buf[i++];
    }

    free(buf);
}

int main(int argc, char **argv) {
    lisp_t *lisp = lisp_new();

    for(int i = 1; i < argc; i++)
        eval_file(lisp, argv[i]);

    eval_file(lisp, "repl.lips");

    exit(EXIT_SUCCESS);
}
