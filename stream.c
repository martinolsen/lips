#include <stdlib.h>

#include "stream.h"
#include "object.h"
#include "logger.h"

int stream_is_eof(object_t * o) {
    if(!object_isa(o, OBJECT_STREAM))
        return 1;

    object_stream_t *s = (object_stream_t *) o;

    if(s->buf_idx >= s->buf_sz)
        return 1;

    return 0;
}

char stream_read_char(object_t * o) {
    if(!object_isa(o, OBJECT_STREAM))
        return EOF;

    if(stream_is_eof(o))
        return EOF;

    object_stream_t *s = (object_stream_t *) o;

    return s->buf[s->buf_idx++];
}

void stream_unread_char(object_t * o, char x) {
    if(!object_isa(o, OBJECT_STREAM))
        PANIC("cannot unread char to non-stream object");

    object_stream_t *s = (object_stream_t *) o;

    if(s->buf_idx == 0)
        PANIC("unreading beyond beginning of stream not supported");

    s->buf[--s->buf_idx] = x;
}
