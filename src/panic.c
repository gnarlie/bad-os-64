#include "common.h"
#include "console.h"

void panic(const char* why) {
    console_print_string("PANIC: ");
    console_print_string(why);

    asm ("cli");
    asm ("hlt");
}

void warn(const char * what) {
    console_print_string("WARN :");
    console_print_string(what);
    console_print_string("\n");
}
