#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"

#define BUFSZ 1024

struct source {
    size_t len;
    const char *text;
};

static struct source *read_source(void) {
    struct source *src = calloc(1, sizeof(struct source));

    if(src == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    src->text = calloc(BUFSZ, sizeof(char));
    src->len = BUFSZ;

    fread((void *) src->text, sizeof(char), BUFSZ, stdin);

    return src;
}

int main(void) {
    fprintf(stderr, "       LISP  Interactive  Performance  System\n");

    struct source *src = read_source();

    lisp_t *lisp = lisp_new();

    object_t *robj = lisp_read(src->text, src->len);

    object_t *eobj = lisp_eval(lisp, NULL, robj);

    printf("res: %s\n", lisp_print(eobj));

    exit(EXIT_SUCCESS);
}
