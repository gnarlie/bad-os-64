#include "tinytest.h"
#include "memory.h"

void panic(const char * why) {
    printf("PANIC %s\n", why);
    ASSERT(why, 0);
}

void disable_interrupts() {}
void enable_interrupts() {}

void register_interrupt_handler(int interrupt, void* handler) { }

void * block;

int main(int argc, char**argv) {
    int rc = 0;

    block = malloc(48*1024*1024);
    kmem_add_block((uint64_t)block, 48*1024*1024, 0x100);

    for (struct tt_test* t = tt_head; t; t = t->next) {
        tt_execute(t->name, t->test_function);
        if (!t->next || 0 != strcmp(t->next->suite, t->suite)) {
            rc |= TEST_REPORT();
        }
    }

    return rc;
}

