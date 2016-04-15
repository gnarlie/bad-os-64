#include "common.h"
#include "console.h"

void panic(const char* why) {
    console_print_string("PANIC: %s", why);

    asm ("cli");
    asm ("hlt");
}

void warn(const char * what) {
    console_print_string("WARN : %s\n", what);
}
