#ifndef __STREAM_H
#define __STREAM_H

#include "object.h"

object_t *istream_mem(const char *, size_t);
object_t *istream_file(const char *);
object_t *ostream_mem(void);
object_t *ostream_file(const char *);

int stream_eof(object_t *);
int stream_read_char(object_t *);
void stream_unread_char(object_t *, int);
void stream_write_char(object_t *, int);
void stream_write_str(object_t *, object_t *);
void stream_close(object_t *);

#endif
