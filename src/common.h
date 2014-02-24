
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef unsigned long size_t;

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

void bzero(void * dest, size_t count);

void panic(const char * why);
