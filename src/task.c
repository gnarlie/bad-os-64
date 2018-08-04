#include "task.h"
#include "memory.h"
#include "interrupt.h"

static Task * head;
static Task * tail;

Task * task_alloc(tasklet callback, void * user) {
    Task * task = (Task*)kmem_alloc(sizeof(Task));
    task->task = callback;
    task->next = NULL;
    task->refs = 0;
    task->user = user;

    return task;
}

void task_enqueue_easy(tasklet t, void * user) {
    Task * task = task_alloc(t, user);
    task_enqueue(task);
}

void task_enqueue(Task * task) {
    if (task->next || head == task) return; // already enqueued.

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

Task* task_get() {
    if (head) {
        disable_interrupts(); //replace with a proper lock

        Task * tmp = head;
        head = head->next;
        enable_interrupts();
        tmp->next = NULL;
        return tmp;
    }

    return NULL;
}

void task_poll_for_work() {
    Task* t;
    while((t = task_get())) {
        t->task(t->user);
        release_ref(t, kmem_free);
    }
}

