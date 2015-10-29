#include "common.h"
#include "memory.h"

typedef int PayloadT;
typedef int KeyT;
typedef int (*HashFun)(KeyT);

struct hash_bucket {
    struct hash_bucket* next;
    KeyT key;
    PayloadT payload;
};

struct hash_table {
    struct hash_bucket** buckets;
    uint32_t size;
    HashFun fn;
};

inline int hash(PayloadT p) {
    return (int) p;
}

inline void hash_init(struct hash_table* table, uint32_t count) {
    uint32_t size = count * sizeof(void*);
    table->buckets = kmem_alloc(size);
    table->size = count;
    table->fn = hash;
    bzero(table->buckets, size);
}

inline void hash_insert(struct hash_table* table, KeyT key, PayloadT value) {
    struct hash_bucket* bucket =
        table->buckets[table->fn(value) % table->size];

    struct hash_bucket* trail = bucket;
    while( bucket ) {
        if (bucket->key == key) break;
    }

    if (!bucket) {
        trail->next = kmem_alloc(sizeof(struct hash_bucket));
        bucket = trail->next;
        bucket->next = NULL;
    }
}
