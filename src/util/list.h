#pragma once

#include "memory.h"

typedef struct list_node {
    struct list_node * prev;
    struct list_node * next;
    void * payload;
} list_node;

typedef struct list {
    list_node * head;
    list_node * tail;
} list_t;

list_t list_create();
list_node * list_append(list_t * list, void * payload);
void list_remove_node(list_t * list, list_node * node);

