#ifndef __LIST_H
#define __LIST_H

typedef struct list_node_t {
    struct list_node_t *prev;
    struct list_node_t *next;

    const void *object;
} list_node_t;

typedef struct {
    list_node_t *first;
    list_node_t *last;
} list_t;

list_t *list_new();
void list_destroy(list_t *);

list_node_t *list_node_new(const void *);

void list_push(list_t *, list_node_t *);
list_node_t *list_pop(list_t *);

#endif
