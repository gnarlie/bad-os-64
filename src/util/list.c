#include "util/list.h"

list_t list_create() {
    list_t l = {.head = NULL, .tail = NULL};
    return l;
}

void list_destroy(list_t *list) {
    while(list->head) {
        list_remove_node(list, list->head);
    }
}

list_node * list_append(list_t * list, void * payload) {
    list_node * node = kmem_alloc(sizeof(list_node));
    node->payload = payload;

    if (!list->tail) {
        list->tail = list->head = node;
        node->next = node->prev = NULL;
    }
    else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
        node->next = NULL;
    }

    return node;
}

void list_remove_node(list_t * list, list_node * node) {
    if (list->head == node) list->head = node->next;
    if (list->tail == node) list->tail = node->prev;

    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;

    kmem_free(node);
}

