#include "common.h"

void outl(uint16_t port, uint32_t value) {
    asm volatile ("outl %1, %0" :: "dN" (port), "a" (value));
}

void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %1, %0" :: "dN" (port), "a" (value));
}

void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1, %0" :: "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void bzero(void *dest, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        ((char*)dest)[i] = 0;
    }
}

char * strncpy(char* dest, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) {
        dest[i] = src[i];
    }

    if (i < n)
        dest[i] = 0;

    return dest;
}

void * memcpy(void * dest, const void * src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char*)dest)[i] = ((char*)src)[i];
    }
    return dest;
}

size_t strlen(const char * what) {
    const char * p = what;
    for (; *p; ++p) ;
    return p  - what;
}

