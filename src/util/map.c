#include "util/map.h"

struct _bucket_entry {
    int key;
    void * value;
};

struct _map_bucket {
    list_t data;
};

int map_int_hash(int i) {
    return i;
}

static void _bucket_add(struct _map_bucket* bucket, int key, char * value) {
    list_node * node;
    LIST_FOREACH(bucket->data, node) {
        struct _bucket_entry * e = node->payload;
        if (e->key == key) {
            e->value = value;
            return;
        }
    }

    struct _bucket_entry * item = kmem_alloc(sizeof(struct _bucket_entry));
    item->key = key;
    item->value = value;
    list_append(&bucket->data, item);
}

void map_init(struct map_t* map, map_hasher hash) {
    map->hash = hash;

    map->n_buckets = 191;
    map->data = kmem_alloc(sizeof(struct _map_bucket) * map->n_buckets);
    bzero(map->data, sizeof(struct _map_bucket) * map->n_buckets);
}

void map_remove(map_t *map, int key) {
    int hash = map->hash(key) % map->n_buckets;
    struct _map_bucket * bucket = &map->data[hash];

    list_node * node;
    if (bucket) {
        LIST_FOREACH(bucket->data, node) {
            struct _bucket_entry * e = node->payload;
            if (e->key == key) {
                list_remove_node(&bucket->data, node);
                return;
            }
        }
    }
}

void map_add(map_t * map, int key, void* value) {
    int hash = map->hash(key) % map->n_buckets;
    struct _map_bucket * bucket = &map->data[hash];

    _bucket_add(bucket, key, value);
}

void * map_lookup(map_t * map, int key) {
    int hash = map->hash(key) % map->n_buckets;
    struct _map_bucket * bucket = &map->data[hash];

    list_node * node;
    LIST_FOREACH(bucket->data, node) {
        struct _bucket_entry * e = node->payload;
        if (e->key == key) return e->value;
    }

    return NULL;
}

void map_destroy(map_t *map) {
    int i;
    for (i = 0; i < map->n_buckets; ++i) {
        list_node * node;
        LIST_FOREACH(map->data[i].data, node) {
            kmem_free(node->payload);
        }
        list_destroy(&map->data[i].data);
    }
}
