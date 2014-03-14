#pragma once

#include "common.h"

typedef void (*tasklet)(void*);

typedef struct TaskT {
    tasklet task;
    void * user;
    uint32_t refs;
    struct TaskT* next;
} Task;

void task_enqueue_easy(tasklet task, void * user);
void task_enqueue(Task *);
Task * task_alloc(tasklet task, void* user);

Task* task_get();

void task_poll_for_work();
