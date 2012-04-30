#ifndef __STREAM_H
#define __STREAM_H

#include "object.h"

object_t *stream_string(const char *, size_t);
object_t *stream_file(const char *);

int stream_is_eof(object_t *);
char stream_read_char(object_t *);
void stream_unread_char(object_t *, char);

#endif
