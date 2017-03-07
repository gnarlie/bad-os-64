#include "task.h"
#include "memory.h"
#include "tinytest/tinytest.h"

void bump(void* user) {
    (*(int*)user) ++;
}

TEST(runSomeTasks) {
    int bumpCount = 0;
    uint32_t start = kmem_current_objects();

    //enqueue and run a task
    task_enqueue(task_alloc(bump, &bumpCount));
    ASSERT_EQUALS(bumpCount, 0);
    task_poll_for_work();
    ASSERT("no leaks", start == kmem_current_objects());
    ASSERT_EQUALS(bumpCount, 1);

    // dont enqueue, nothing runs
    task_poll_for_work();
    ASSERT_EQUALS(bumpCount, 1);

    // enqueue two
    task_enqueue(task_alloc(bump, &bumpCount));
    task_enqueue(task_alloc(bump, &bumpCount));
    task_poll_for_work();

    ASSERT_EQUALS(bumpCount, 3);
    ASSERT("no leaks", start == kmem_current_objects());

    Task * t = task_alloc(bump, &bumpCount);
    add_ref(t);
    task_enqueue(t);
    task_poll_for_work();
    release_ref(t, kmem_free);
    ASSERT("no leaks", start == kmem_current_objects());
}

TEST(tasksDontDoubleQueue) {
    int bumpCount = 0;
    uint32_t start = kmem_current_objects();

    Task * t2 = task_alloc(bump, &bumpCount);
    add_ref(t2);
    task_enqueue(t2);
    task_enqueue(t2);
    task_poll_for_work();

    release_ref(t2, kmem_free);
    ASSERT_EQUALS(1, bumpCount);
    ASSERT("no leaks", start == kmem_current_objects());
}

