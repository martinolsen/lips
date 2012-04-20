#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "logger.h"

int logger(const char *lvl, const char *file, const int line,
           const char *fmt, ...) {

    va_list ap;

    const size_t len = 1024;
    char *s = calloc(len, sizeof(char));

    va_start(ap, fmt);
    vsnprintf(s, len, fmt, ap);
    va_end(ap);

    return fprintf(stderr, "%s[%s:%03d] - %s\n", lvl, file, line, s);
}
