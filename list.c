#include <stdio.h>
#include <stdlib.h>

#include "list.h"

list_t *list_new() {
    list_t *list = calloc(1, sizeof(list_t));

    if(list == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    return list;
}

void list_destroy(list_t * list) {
    free(list);
}

list_node_t *list_node_new(const void *object) {
    list_node_t *node = calloc(1, sizeof(list_node_t));

    if(node == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    node->object = object;

    return node;
}

void list_push(list_t * list, list_node_t * node) {
    if(list->last != NULL) {
        node->prev = list->last;
        list->last->next = node;
    }

    list->last = node;

    if(list->first == NULL)
        list->first = node;
}

list_node_t *list_pop(list_t * list) {
    if(list->last == NULL)
        return NULL;

    list_node_t *end = list->last;

    list->last = end->prev;

    if(list->last != NULL)
        list->last->next = NULL;

    end->next = NULL;
    end->prev = NULL;

    return end;
}
