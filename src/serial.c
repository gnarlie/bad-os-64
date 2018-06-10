#include "serial.h"
#include "common.h"

static int is_transmit_empty(int port) {
    return inb(port + 5) & 0x20;
}

void serial_put(int com, char c) {
    while (!is_transmit_empty(com)) ;

      outb(com, c);
}

