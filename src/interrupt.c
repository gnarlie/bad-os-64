#include "interrupt.h"

static isr_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(uint8_t n, registers_t * regs) {
    isr_t handler = interrupt_handlers[n];
    if (handler) {
        handler(regs);
    }
    else {
        panic("unhandled exception");
    }
}

void irq_handler(uint8_t intNo, registers_t * regs) {
    isr_t handler = interrupt_handlers[intNo];
    if (handler) {
        handler(regs);
    }
}

extern void create_gate(int, void(*)());

void init_interrupts() {
    //tell the pic to enable more interupts (Pure64 just did cascade, keyboard, and RTC)
    outb(0x21, 0xf8);
//    outb(0xa1, 0);

#define DO_IRQ(x,y)   extern void irq##x(); create_gate(y, irq##x);
    DO_IRQ(0, 32);
    DO_IRQ(1, 33);
    DO_IRQ(2, 34);
    DO_IRQ(3, 35);
    DO_IRQ(4, 36);
    DO_IRQ(5, 37);
    DO_IRQ(6, 38);
    DO_IRQ(7, 39);
    DO_IRQ(8, 40);
    DO_IRQ(9, 41);
    DO_IRQ(10, 42);
    DO_IRQ(11, 43);
    DO_IRQ(12, 44);
    DO_IRQ(13, 45);
    DO_IRQ(14, 46);
    DO_IRQ(15, 47);
#undef DO_IRQ

#define DO_ISR(x) extern void isr##x(); create_gate(x, isr##x);
    DO_ISR(0);
    DO_ISR(1);
    DO_ISR(2);
    DO_ISR(3);
    DO_ISR(4);
    DO_ISR(5);
    DO_ISR(6);
    DO_ISR(7);
    DO_ISR(8);
    DO_ISR(9);
    DO_ISR(10);
    DO_ISR(11);
    DO_ISR(12);
    DO_ISR(13);
    DO_ISR(14);
    DO_ISR(15);
    DO_ISR(16);
    DO_ISR(17);
    DO_ISR(18);
    DO_ISR(19);
#undef DO_ISR
}

void disable_interrupts() {asm("cli");}
void enable_interrupts() {asm("sti");}
