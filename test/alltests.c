#include "tinytest/tinytest.h"
#include "memory.h"
#include "process.h"

void panic(const char * why) {
    printf("PANIC %s\n", why);
    abort();
}

void call_user_function(struct process * proc) {
    proc->entry();
}

void disable_interrupts() {}
void enable_interrupts() {}

void register_interrupt_handler(int interrupt, void* handler) { }

void * block;

int main(int argc, char**argv) {
    block = malloc(48*1024*1024);
    kmem_add_block((void*)block, 48*1024*1024, 0x100);

    return tt_run_all();
}

