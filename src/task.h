#pragma once

#include "common.h"

typedef void (*tasklet)();

typedef struct NodeT {
    tasklet task;
    uint32_t refs;
    struct NodeT* next;
} Task;

void task_enqueue_easy(tasklet task);
void task_enqueue(Task *);
Task * task_alloc(tasklet task);

tasklet task_get();

void task_poll_for_work();
