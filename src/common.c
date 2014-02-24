#include "common.h"

void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1, %0" :: "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void bzero(void *dest, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        ((char*)dest)[i] = 0;
    }
}

