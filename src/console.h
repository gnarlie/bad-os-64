#pragma once

#include "common.h"

enum Color {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    Gray = 7,
    Bright = 8
};

extern void console_set_color(enum Color fore, enum Color back);

extern void console_put(char c);
extern void console_print_string(const char*, ...);
extern void console_put_hex8(uint8_t v);
extern void console_put_hex16(uint16_t v);
extern void console_put_hex(uint32_t v);
extern void console_put_hex64(uint64_t v);
extern void console_put_dec(uint32_t v);
extern void console_clear_screen();
