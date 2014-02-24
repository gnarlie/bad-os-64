#include "common.h"
#include "console.h"
#include "keyboard.h"
#include "memory.h"
#include "rtc.h"

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

static isr_t interrupt_handlers[256];

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

void protection(registers_t* regs) {
    console_print_string("gpf\n");
    dump_regs(regs);
}

void timer_irq(registers_t* regs) {
    static uint32_t time;
    uint32_t now = read_rtc();
    if (time != now) {
        time = now;

        char * clock = (char*) 0xb8000 + 2 * (80 - 8);
        uint32_t hr = time / 3600;
        clock[0] = (hr / 10) + '0';
        clock[2] = (hr % 10) + '0';
        clock[4] = ':';
        uint32_t min = time / 60 % 60;
        clock[6] = (min / 10) + '0';
        clock[8] = (min % 10) + '0';
        clock[10] = ':';
        uint32_t sec = time % 60;
        clock[12] = (sec / 10) + '0';
        clock[14] = (sec % 10) + '0';
    }
}

void main() {
    console_print_string("Hello from C\n");
    init_interrupts();

    kmem_init();
    // need to make these less arbiratry, based on the
    // actual memory map
    kmem_add_block(0x200000, 1024*1024*1024, 0x400);

    uint32_t lapicBase = *(uint32_t*)0x5060;
    console_print_string("LAPIC ");
    console_put_hex64(lapicBase);
    console_print_string(". CPU Speed ");
    uint16_t cpuSpeed = *(uint16_t*)0x5010;
    console_put_dec(cpuSpeed);
    console_print_string("MHz \n");

    register_interrupt_handler(IRQ1, keyboard_irq);
    register_interrupt_handler(3, breakpoint);
    register_interrupt_handler(0xd, protection);

    // try a breakpoint
    uint64_t here;
    asm volatile ("lea (%%rip), %0" : "=r" (here) );
    asm volatile ("int $3");
    console_print_string("breakpoint was just after: ");
    console_put_hex64(here);

    //enable the timer
    register_interrupt_handler(IRQ0, timer_irq);
}
