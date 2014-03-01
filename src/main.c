#include "common.h"
#include "console.h"
#include "keyboard.h"
#include "memory.h"
#include "rtc.h"
#include "interrupt.h"
#include "task.h"
#include "pci.h"
#include "ne2k.h"

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
    console_print_string("ds "); console_put_hex(regs->ds);
    console_print_string(" es "); console_put_hex(regs->es);
    console_print_string(" fs "); console_put_hex(regs->fs);
    console_print_string(" gs "); console_put_hex(regs->gs);
    console_print_string("\n");
    console_print_string("rip "); console_put_hex64(regs->rip);
    console_print_string(" cs  "); console_put_hex(regs->cs);
    console_print_string("\n");
    console_print_string("rflags "); console_put_hex64(regs->rflags);
    console_print_string(regs->rflags & 1 << 14 ? " N" : " n");
    console_print_string(regs->rflags & 1 << 11 ? "O" : "o");
    console_print_string(regs->rflags & 1 << 10 ? "D" : "d");
    console_print_string(regs->rflags & 1 << 9 ? "I" : "i");
    console_print_string(regs->rflags & 1 << 8 ? "T" : "t");
    console_print_string(regs->rflags & 1 << 7 ? "S" : "s");
    console_print_string(regs->rflags & 1 << 6 ? "Z" : "z");
    console_print_string(regs->rflags & 1 << 4 ? "A" : "a");
    console_print_string(regs->rflags & 1 << 2 ? "P" : "p");
    console_print_string(regs->rflags & 1 ? "C" : "c");
    console_print_string(" IOPL="); console_put_hex((regs->rflags >> 12) & 3);
    console_print_string("\n");
    console_print_string("error "); console_put_hex64(regs->errorCode);
    console_print_string("\n");
    console_print_string("rbp "); console_put_hex64(regs->rbp);
    console_print_string("\n");
}

void breakpoint(registers_t* regs) {
    console_print_string("breakpoint!\n");
    dump_regs(regs);
}

void protection(registers_t* regs) {
    console_print_string("gpf\n");
    dump_regs(regs);
    panic("cannot continue");
}

static void update_clock() {
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

Task * update_task;
void timer_irq(registers_t* regs) {
    task_enqueue(update_task);
}

void main() {
    console_print_string("Hello from C\n");
    init_interrupts();

    kmem_init();

    uint32_t ram = *(uint32_t*)0x5020;
    console_print_string("System RAM: ");
    console_put_dec(ram);
    console_print_string("MB. CPU Speed ");
    uint16_t cpuSpeed = *(uint16_t*)0x5010;
    console_put_dec(cpuSpeed);
    console_print_string("MHz \n");

    // need to make these less arbiratry, based on the
    // actual memory map
    kmem_add_block(0x200000, 1024*1024*1024, 0x400);

    init_pci();
    init_ne2k();

    init_keyboard();
    register_interrupt_handler(3, breakpoint);
    register_interrupt_handler(0xd, protection);

    // try a breakpoint
//    uint64_t here;
//    asm volatile ("lea (%%rip), %0" : "=r" (here) );
//    asm volatile ("int $3" ::: "memory" );
//    console_print_string("breakpoint was just after: ");
//    console_put_hex64(here);
//    console_print_string("\n");

    //enable the timer and display a clock
    update_task = task_alloc(update_clock);
    add_ref(update_task);
    register_interrupt_handler(IRQ0, timer_irq);
}
