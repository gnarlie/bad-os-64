#include "common.h"
#include "memory.h"

#include "util/list.h"

typedef int (*map_hasher)(int);

struct _map_bucket;

typedef struct map_t {
    struct _map_bucket * data;
    uint32_t n_buckets;
    map_hasher hash;
} map_t;


int map_int_hash(int i);

void map_init(struct map_t* map, map_hasher hash);
void map_add(map_t * map, int key, void* value);
void map_destroy(map_t *map);
void* map_lookup(map_t * map, int key);
void map_remove(map_t *map, int key);

