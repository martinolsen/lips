#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "stream.h"
#include "object.h"
#include "logger.h"

static void fd_writer(object_stream_t *, int);
static int fd_reader(object_stream_t *);
static void fd_unreader(object_stream_t *, int);
static void fd_closer(object_stream_t *);

object_t *istream_mem(const char *str, size_t len) {
    FILE *fd = fmemopen((void *) str, len, "r");

    if(fd == NULL) {
        perror("fmemopen");
        exit(EXIT_FAILURE);
    }

    return object_stream_new(fd, fd_reader, fd_unreader, NULL, fd_closer);
}

object_t *istream_file(const char *path) {
    FILE *fd = fopen(path, "r");

    if(fd == NULL) {
        perror("fmemopen");
        exit(EXIT_FAILURE);
    }

    return object_stream_new(fd, fd_reader, fd_unreader, NULL, fd_closer);
}

object_t *ostream_mem(void) {
    return NULL;
}

object_t *ostream_file(const char *path) {
    FILE *fd = fopen(path, "w");

    if(fd == NULL) {
        perror("fmemopen");
        exit(EXIT_FAILURE);
    }

    return object_stream_new(fd, NULL, NULL, fd_writer, fd_closer);
}

int stream_eof(object_t * o) {
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

int stream_read_char(object_t * o) {
    if(!object_isa(o, OBJECT_STREAM))
        PANIC("cannot read char to non-stream object");

    object_stream_t *stream = (object_stream_t *) o;

    if(stream->read == NULL)
        PANIC("stream reader not defined");

    return stream->read(stream);
}

void stream_unread_char(object_t * o, int c) {
    if(!object_isa(o, OBJECT_STREAM))
        PANIC("cannot unread char to non-stream object");

    object_stream_t *stream = (object_stream_t *) o;

    if(stream->unread == NULL)
        PANIC("stream unreader not defined");

    stream->unread(stream, c);
}

void stream_write_char(object_t * o, int c) {
    if(!object_isa(o, OBJECT_STREAM))
        PANIC("cannot write char to non-stream object");

    object_stream_t *stream = (object_stream_t *) o;

    if(stream->write == NULL)
        PANIC("stream writer not defined");

    return stream->write(stream, c);
}

void stream_close(object_t * o) {
    if(!object_isa(o, OBJECT_STREAM))
        PANIC("cannot close non-stream object");

    object_stream_t *stream = (object_stream_t *) o;

    if(stream->close == NULL)
        PANIC("stream closer not defined");

    return stream->close(stream);
}

static int fd_reader(object_stream_t * stream) {
    if(stream == NULL)
        PANIC("stream is null!");

    if(stream->fd == NULL)
        PANIC("stream->fd is null!");

    int c = fgetc(stream->fd);

    if(c == EOF && !feof(stream->fd)) {
        perror("fgetc");
        exit(EXIT_FAILURE);
    }

    return c;
}

static void fd_unreader(object_stream_t * stream, int c) {
    if(stream == NULL)
        PANIC("stream is null!");

    if(stream->fd == NULL)
        PANIC("stream->fd is null!");

    if(EOF == ungetc(c, stream->fd)) {
        perror("ungetc");
        exit(EXIT_FAILURE);
    }
}

static void fd_writer(object_stream_t * stream, int c) {
    if(stream == NULL)
        PANIC("stream is null!");

    if(stream->fd == NULL)
        PANIC("stream->fd is null!");

    if(EOF == fputc(c, stream->fd)) {
        perror("fputc");
        exit(EXIT_FAILURE);
    }
}

static void fd_closer(object_stream_t * stream) {
    if(stream == NULL)
        PANIC("stream is null!");

    if(stream->fd == NULL)
        PANIC("stream->fd is null!");

    if(0 != fclose(stream->fd)) {
        perror("fclose");
        exit(EXIT_FAILURE);
    }
}
