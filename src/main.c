#include "common.h"
#include "console.h"
#include "keyboard.h"

#define IRQ0 32
#define IRQ1 33

typedef struct registers {
    uint64_t ds;
    uint64_t es;
    uint64_t fs;
    uint64_t gs;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t errorCode;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
} registers_t;

typedef void (*isr_t)(registers_t*);

isr_t interrupt_handlers[256];

// isr impl
void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(uint8_t n, registers_t * regs) {
    isr_t handler = interrupt_handlers[n];
    if (handler) {
        handler(regs);
    }
    else {
        console_put_hex(n);
        console_print_string(" unhandled exception\n");
    }
}

void irq_handler(uint8_t intNo, registers_t * regs) {
    isr_t handler = interrupt_handlers[intNo];
    if (handler) {
        handler(regs);
    }
}

extern void irq1();
extern void create_gate(int, void(*)());
void init_interrupts() {
    create_gate(IRQ1, irq1);

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
}

void dump_regs(registers_t* regs) {
    console_print_string("rax "); console_put_hex64(regs->rax);
    console_print_string(" rbx "); console_put_hex64(regs->rbx);
    console_print_string("\n");
    console_print_string("rcx "); console_put_hex64(regs->rcx);
    console_print_string(" rdx "); console_put_hex64(regs->rdx);
    console_print_string("\n");
    console_print_string("r8  "); console_put_hex64(regs->r8);
    console_print_string(" r9  "); console_put_hex64(regs->r9);
    console_print_string("\n");
    console_print_string("r10 "); console_put_hex64(regs->r10);
    console_print_string(" r11 "); console_put_hex64(regs->r11);
    console_print_string("\n");
    console_print_string("r12 "); console_put_hex64(regs->r12);
    console_print_string(" r13 "); console_put_hex64(regs->r13);
    console_print_string("\n");
    console_print_string("r14 "); console_put_hex64(regs->r14);
    console_print_string(" r15 "); console_put_hex64(regs->r15);
    console_print_string("\n");
    console_print_string("rsi "); console_put_hex64(regs->rsi);
    console_print_string(" rdi "); console_put_hex64(regs->rdi);
    console_print_string("\n");
    console_print_string("ds  "); console_put_hex64(regs->ds);
    console_print_string(" es  "); console_put_hex64(regs->es);
    console_print_string("\n");
    console_print_string("fs  "); console_put_hex64(regs->fs);
    console_print_string(" gs  "); console_put_hex64(regs->gs);
    console_print_string("\n");
    console_print_string("rip "); console_put_hex64(regs->rip);
    console_print_string(" cs  "); console_put_hex64(regs->cs);
    console_print_string("\n");
    console_print_string("rflags "); console_put_hex64(regs->rflags);
    console_print_string("\n");
}

void breakpoint(registers_t* regs) {
    console_print_string("breakpoint!\n");
    dump_regs(regs);
}

void main() {
    console_print_string("Hello from C\n");
    init_interrupts();

    console_put_hex(0xdeadbeef);
    console_put_dec(1337);
    console_print_string("\n");

    register_interrupt_handler(IRQ1, keyboard_irq);
    register_interrupt_handler(3, breakpoint);

    uint64_t here;
    asm volatile ("lea (%%rip), %0" : "=r" (here) );
    console_put_hex64(here);

    asm volatile ("int $3");
}
