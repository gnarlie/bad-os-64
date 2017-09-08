#include "tinytest/tinytest.h"
#include "memory.h"

void panic(const char * why) {
    printf("PANIC %s\n", why);
    ASSERT(why, 0);
}

void call_user_function(void * fn) {
    FAIL("Not implemented");
}

void disable_interrupts() {}
void enable_interrupts() {}

void register_interrupt_handler(int interrupt, void* handler) { }

void * block;

int main(int argc, char**argv) {
    block = malloc(48*1024*1024);
    kmem_add_block((uint64_t)block, 48*1024*1024, 0x100);

    return tt_run_all();
}

