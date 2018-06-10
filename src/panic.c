#include "common.h"
#include "console.h"

void panic(const char* why) {
    console_set_color(Red, Black);
    console_print_string("PANIC: %s", why);

    asm ("xchgw %bx, %bx");
    asm ("cli");
    asm ("hlt");
}

void warn(const char * what) {
    console_set_color(Bright | Brown, Black);
    console_print_string("WARN : %s\n", what);
    console_set_color(Gray, Black);
}
