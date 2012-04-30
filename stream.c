#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "stream.h"
#include "object.h"
#include "logger.h"

object_t *stream_string(const char *str, size_t len) {
    FILE *fd = fmemopen((void *) str, len, "r");

    if(fd == NULL) {
        perror("fmemopen");
        exit(EXIT_FAILURE);
    }

    return object_stream_new(fd);
}

object_t *stream_file(const char *path) {
    FILE *fd = fopen(path, "r");

    if(fd == NULL) {
        perror("fmemopen");
        exit(EXIT_FAILURE);
    }

    return object_stream_new(fd);
}

int stream_is_eof(object_t * o) {
    if(!object_isa(o, OBJECT_STREAM))
        return 1;

    object_stream_t *s = (object_stream_t *) o;

    if(feof(s->fd))
        return 1;

    int c = fgetc(s->fd);

    ungetc(c, s->fd);

    if(c == EOF)
        return 1;

    return 0;
}

char stream_read_char(object_t * o) {
    if(!object_isa(o, OBJECT_STREAM))
        return EOF;

    if(stream_is_eof(o))
        return EOF;

    object_stream_t *s = (object_stream_t *) o;

    if(s->fd == NULL)
        PANIC("stream's fd is NULL");

    int c = fgetc(s->fd);

    if(c == EOF && !feof(s->fd)) {
        perror("fgetc");
        exit(EXIT_FAILURE);
    }

    return c;
}

void stream_unread_char(object_t * o, char x) {
    if(!object_isa(o, OBJECT_STREAM))
        PANIC("cannot unread char to non-stream object");

    object_stream_t *s = (object_stream_t *) o;

    if(EOF == ungetc(x, s->fd)) {
        perror("ungetc");
        exit(EXIT_FAILURE);
    }
}
