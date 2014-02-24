#include "common.h"

static char *VideoStart = (char *) 0xB8000;
static char *current = (char*) 0xB8000;

static const uint16_t lines = 25;
static const uint16_t cols = 160; // in bytes, 80 columns

typedef unsigned long ptrdiff_t;

static void scroll() {
    char * p = VideoStart;
    int i;
    for (; p < VideoStart + (lines - 1) * cols; p += cols) {
        for(i = 0; i < cols; i++) {
            p[i] = p[i + cols];
        }
    }

    for(i = 0; i < cols; i++) {
        p[i] = p[i + cols];
    }
    for(i = 0; i < cols; i+=2) {
        p[i + cols] = ' ';
    }

    current -= cols;
}

void console_put(char c) {
    switch(c) {
        case('\n'):
            current += cols;
            ptrdiff_t dist = current - VideoStart;
            current = VideoStart + (dist / cols) * cols;
            break;
        case('\b'):
            current -= 2;
            *current = ' ';
            if (current < VideoStart)
                current = VideoStart;
            break;
        default:
            *current = c;
            current += 2;
    }

    if (current > VideoStart + (lines - 1) * cols) {
        scroll();
    }
}

void console_clear_screen() {
    char * p = VideoStart;
    for (; p < VideoStart + lines * cols; p += 2) {
        *p = ' ';
    }

    current = VideoStart;
}

void console_print_string(const char * str) {
    while (*str) {
        console_put(*str++);
    }
}

static char hex(int i) {
    if (i > 16) return '?';
    if (i > 9) return 'A' + (i - 10);
    else return '0' + i;
}

void console_put_hex(uint32_t v) {
    console_put('0');
    console_put('x');
    for (int i = 28; i >= 0; i -= 4) {
        console_put(hex((v >> i) & 0xf ));
    }
}

void console_put_hex64(uint64_t v) {
    console_put('0');
    console_put('x');
    for (int i = 60; i >= 0; i -= 4) {
        console_put(hex((v >> i) & 0xf ));
    }
}

void console_put_dec(uint32_t v) {
    char str[11];
    str[10] = 0;
    if (!v) {
        console_put('0');
        return;
    }

    char * p = str + 9;
    for(; v; p--) {
        *p = v % 10 + '0';
        v /= 10;
    }

    console_print_string(p + 1);
}
