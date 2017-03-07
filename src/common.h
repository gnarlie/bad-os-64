#pragma once

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long  uint64_t;

typedef unsigned long  size_t;

void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outl(uint16_t port, uint32_t value);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void bzero(void * dest, size_t count);
void *memcpy(void * dest, const void * src, size_t n);
int memcmp(const void * a, const void *b, size_t n);

size_t strlen(const char * what);
char * strncpy(char* dest, const char *src, size_t n);
int strncmp(char const * a, const char * b, size_t n);
void strrev(char * str, char * end);
char * strnchr(char * s, int c, size_t n);

void panic(const char * why);
void warn(const char * what);

#ifndef NULL
#define NULL ((void*)0)
#endif

#define add_ref(thing) (thing)->refs++
#define release_ref(thing, free) if (--(thing->refs) == 0) {free(thing);}

void to_str(uint32_t d, char * buff);

