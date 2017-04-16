#include "../tinytest/tinytest.h"

#include "util/map.h"

TEST(map_tests) {
    struct map_t map;
    map_init(&map, map_int_hash);

    const char * a = "a";
    const char * b = "b";
    const char * c = "c";

    map_add(&map, 1, (void*)a);
    map_add(&map, 2, (void*)b);

    ASSERT_STRING_EQUALS(map_lookup(&map, 1), "a");
    ASSERT_STRING_EQUALS(map_lookup(&map, 2), "b");
    ASSERT_EQUALS(map_lookup(&map, 3), NULL);

    map_add(&map, 1, (void*)c);
    ASSERT_STRING_EQUALS("c", map_lookup(&map, 1));

    map_remove(&map, 2);
    ASSERT_EQUALS(map_lookup(&map, 2), NULL);

    map_destroy(&map);
}
