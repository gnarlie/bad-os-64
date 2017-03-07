#include "memory.h"
#include "tinytest/tinytest.h"

TEST(simpleAllocation) {
    extern void * block;

    void * m1 = kmem_alloc(64),
         * m2 = kmem_alloc(333),
         * m3 = kmem_alloc(64);
    ASSERT("in allocation region", m1 > block);
    ASSERT("in allocation region", m2 > block);
    ASSERT("in allocation region", m3 > block);

    ASSERT_EQUALS(0x100, m2 - m1);
    ASSERT_EQUALS(0x200, m3 - m2);

    kmem_free(m1);
    void * m4 = kmem_alloc(64);
    ASSERT_EQUALS(m1, m4);
    kmem_free(m2);
    kmem_free(m3);
}


