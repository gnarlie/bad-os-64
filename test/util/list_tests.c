#include "../tinytest.h"

#include "util/list.h"

#include <stdio.h>

TEST(list_append) {
    list_t l = list_create();
    list_node * a = list_append(&l, 0);
    list_node * b = list_append(&l, 0);

    ASSERT_EQUALS(l.head, a);
    ASSERT_EQUALS(l.tail, b);

    ASSERT_EQUALS(a->next, b);
    ASSERT_EQUALS(b->next, NULL);

    ASSERT_EQUALS(a->prev, NULL);
    ASSERT_EQUALS(b->prev, a);
}

TEST(list_remove) {
    list_t l = list_create();
    list_node * a = list_append(&l, 0);
    list_node * b = list_append(&l, 0);
    list_node * c = list_append(&l, 0);
    list_node * d = list_append(&l, 0);
    list_node * e = list_append(&l, 0);

    list_remove_node(&l, a);
    ASSERT_EQUALS(l.head, b);
    ASSERT_EQUALS(b->prev, NULL);

    list_remove_node(&l, e);
    ASSERT_EQUALS(l.tail, d);
    ASSERT_EQUALS(d->next, NULL);

    list_remove_node(&l, c);
    ASSERT_EQUALS(d->prev, b);
    ASSERT_EQUALS(b->next, d);

    list_remove_node(&l, d);
    list_remove_node(&l, b);

    ASSERT_EQUALS(l.head, NULL);
    ASSERT_EQUALS(l.tail, NULL);
}


