
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define IRQ0 32
#define IRQ1 33

typedef struct registers {
    uint32_t int_no;
} registers_t;

typedef void (*isr_t)(registers_t);

isr_t interrupt_handlers[256];

void outb(uint16_t port, uint8_t value ) {
    asm volatile ("outb %1, %0" :: "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

// isr impl
void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void isr_handler(uint8_t n) {
    isr_t handler = interrupt_handlers[n];
    if (handler) {
        registers_t t;
        t.int_no = n;
        handler(t);
    }
    else {
        print_to_console("unhandled exception");
    }
}

void irq_handler(uint8_t intNo) {
    isr_t handler = interrupt_handlers[intNo];
    if (handler) {
        registers_t t;
        t.int_no = intNo;
        handler(t);
    }
}

extern irq1();
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

void breakpoint() {
    print_to_console("breakpoint!\n");
}

void keyboard_irq() {
    inb(0x60);
    print_to_console("got it\n");
}

void init_keyboard() {
    register_interrupt_handler(IRQ1, keyboard_irq);
}

void main() {
    print_to_console("Hello from C\n");
    init_interrupts();
    print_to_console("Hello from C\n");

    init_keyboard();

    register_interrupt_handler(3, breakpoint);
    asm volatile ("int $3");
    asm volatile ("int $3");
}
