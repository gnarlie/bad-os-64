#include "process.h"
#include "task.h"
#include "memory.h"

#include "tinytest/tinytest.h"

static int called = 0;
static void thing(int ignored) {
    called = 42;
}

TEST(create_process) {
    uint32_t curr = kmem_current_objects();
    process_t * p = create_process(thing);

    ASSERT_INT_EQUALS(0, called);
    task_poll_for_work();
    ASSERT_INT_EQUALS(42, called);

    p->reap(p);
    ASSERT_INT_EQUALS(curr, kmem_current_objects());
}
