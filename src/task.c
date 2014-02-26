#include "task.h"
#include "memory.h"
#include "interrupt.h"

static Task * head;
static Task * tail;

//int lock;
//void lock_init(int* lock) {*lock = 0;}
//
//void lock_aquire(int* lock) {
//    while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
//        while(*lock) asm volatile ("pause");
//}
//
//void lock_release(int* lock) {
//    asm volatile ("");
//    *lock = 0;
//}

Task * task_alloc(tasklet callback) {
    Task * task = (Task*)kmem_alloc(sizeof(Task));
    task->task = callback;
    task->next = NULL;
    task->refs = 0;

    return task;
}

void task_enqueue(Task * task) {
    if (task->next) return; // already enqueued.

    add_ref(task);
    if (!head) {
        tail = head = task;
    }
    else if (tail) {
        tail->next = task;
        tail = task;
    }
    else {
        panic("Head without a tail");
    }
}

tasklet task_get() {
    if (head) {

        disable_interrupts(); //replace with a proper lock
        tasklet t = head->task;

        Task * tmp = head;
        head = head->next;
        enable_interrupts();

        tmp->next = NULL;
        release_ref(tmp, kmem_free);

        return t;
    }

    return NULL;
}

void task_poll_for_work() {
    tasklet t;
    while((t = task_get())) {
        t();
    }
}

