#include "tinytest.h"

void panic(const char * why) {
    ASSERT(why, 0);
}

void disable_interrupts() {}
void enable_interrupts() {}

int main(int argc, char**argv) {
    int rc = 0;
    RUN(simpleAllocation); rc |= TEST_REPORT();

    RUN(runSomeTasks);
    RUN(tasksDontDoubleQueue); rc |= TEST_REPORT();

    return rc;
}

