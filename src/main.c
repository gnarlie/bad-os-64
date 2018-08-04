#include "common.h"
#include "console.h"
#include "keyboard.h"
#include "memory.h"
#include "interrupt.h"
#include "task.h"
#include "pci.h"
#include "ne2k.h"
#include "ata.h"
#include "entry.h"
#include "process.h"

#include "fs/vfs.h"

#include "service/http.h"
#include "service/echo.h"
#include "service/clock.h"

static void dump_stack(registers_t* regs) {
    uint32_t * start = (uint32_t*)(regs + 1);
    uint32_t * p = start;
    for(; p < start + 16; p += 4) {
        console_print_string("%p: %x %x %x %x\n", p, p[0], p[1], p[2], p[3]);
    }
}

static void dump_regs(registers_t* regs) {
    console_print_string("rax %p rbx %p\n", regs->rax, regs->rbx);
    console_print_string("rcx %p rdx %p\n", regs->rcx, regs->rdx);
    console_print_string("r8  %p r9  %p\n", regs->r8, regs->r9);
    console_print_string("r10 %p r11 %p\n", regs->r10, regs->r11);
    console_print_string("r12 %p r13 %p\n", regs->r12, regs->r13);
    console_print_string("r14 %p r15 %p\n", regs->r14, regs->r15);
    console_print_string("rsi %p rdi %p\n", regs->rsi, regs->rdi);
    console_print_string("ds %x es %x fs %x gs %x\n",
            (uint32_t)regs->ds, (uint32_t)regs->es, (uint32_t)regs->fs, (uint32_t)regs->gs);
    console_print_string("rip "); console_put_hex64(regs->rip);
    console_print_string(" cs  "); console_put_hex(regs->cs);
    console_print_string("\n");
    console_print_string("rflags %p", regs->rflags);
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
    console_print_string(" IOPL=%x\n", (uint32_t)((regs->rflags >> 12) & 3));
    console_print_string("error %p\n", regs->errorCode);
    console_print_string("ss %x rsp %p - rbp %p\n", (uint32_t)regs->ss, regs + 1, regs->rbp);

    uint64_t cr0, cr2, cr3, cr4;
    __asm__ __volatile__ ("movq %%cr0, %%rax\n\t movq %%rax, %0\n\t"
                          "movq %%cr2, %%rax\n\t movq %%rax, %1\n\t"
                          "movq %%cr3, %%rax\n\t movq %%rax, %2\n\t"
                          "movq %%cr4, %%rax\n\t movq %%rax, %3\n\t":
            "=m"(cr0), "=m"(cr2), "=m"(cr3), "=m"(cr4)::"rax");
    console_print_string("cr0 %p cr2 %p\n", cr0, cr2);
    console_print_string("cr3 %p cr4 %p\n", cr3, cr4);
}

static void double_fault(registers_t* regs, void*user) {
    panic("double fault!");
}

static void breakpoint(registers_t* regs, void*user) {
    console_print_string("breakpoint!\n");
    dump_regs(regs);
    dump_stack(regs);
}

static void protection(registers_t* regs, void*user) {
    console_print_string("General protection fault\n");
    dump_regs(regs);
    dump_stack(regs);
    panic("cannot continue");
}

static void page_fault(registers_t* regs, void*user) {

    enum PageFaultFlags {
        p = 1,
        wr = 2,
        us = 4,
        rsvd = 8,
        id = 16
    };

    uint64_t cr2;
    __asm__ __volatile__ ("movq %%cr2, %%rax\n\t movq %%rax, %0\n\t": "=m"(cr2) :: "rax" );

    console_print_string("page fault at %x\n", cr2);
    console_print_string("cause: %s %s %s (%x)\n",
            regs->errorCode & p ? "protection" : "non-present page",
            regs->errorCode & wr ? "write" : "read",
            regs->errorCode & us ? "user" : "supervisor",
            regs->errorCode
            );

    dump_regs(regs);
    dump_stack(regs);

    panic("cannot continue");
}

/****
 * Starting with the boot loader's GDT, which is good enough for kernel
 * operation - extend to allow user mode
 */
static void init_gdt() {

    struct gdt_entry gdt[8];
    bzero(gdt, sizeof(gdt));

    // gdt[0] is null selector ... leave 0's

    // gdt[1] is kernel code
    gdt[1].granularity = 1; // limit in pages
    gdt[1].l = 1;           // 64bit
    gdt[1].present = 1;     // valid
    gdt[1].descType = 1;    // must be 1
    gdt[1].type = 0x8;      // execute / read
    gdt[1].lowLimit = 0xffff;
    gdt[1].segLimit = 0xf;

    // gdt[2] is kernel data
    gdt[2] = gdt[1];
    gdt[2].type = 2;        // read / write
    gdt[2].l = 0;
    gdt[2].db = 1;

    // gdt[3] is another null selector (s/b 32 bit code)
    gdt[3] = gdt[0];

    // gdt[4] is user data
    gdt[4] = gdt[2];
    gdt[4].privLvl = 3;

    // gdt[5] is user code
    gdt[5] = gdt[1];
    gdt[5].privLvl = 3;

    // gdt[6] is the TSS descriptor
    create_tss(&gdt[6]);

    install_gdt(gdt, sizeof(gdt));
    install_tss();
}

static void user_mode() {
    syscall(console_print_string, "hello from ring 3\n");
}

static void cpu_details() {
    uint32_t eax, ebx, ecx, edx;
    char brand[48];
    bzero(brand, sizeof(brand));

    asm volatile ("mov %4, %%eax\ncpuid":
            "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx): "g"(0x80000002));
    memcpy(brand, &eax, 4);
    memcpy(brand+4, &ebx, 4);
    memcpy(brand+8, &ecx, 4);
    memcpy(brand+12, &edx, 4);

    asm volatile ("mov %4, %%eax\ncpuid":
            "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx): "g"(0x80000003));
    memcpy(brand+16, &eax, 4);
    memcpy(brand+20, &ebx, 4);
    memcpy(brand+24, &ecx, 4);
    memcpy(brand+28, &edx, 4);

    asm volatile ("mov %4, %%eax\ncpuid":
            "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx): "g"(0x80000004));
    memcpy(brand+32, &eax, 4);
    memcpy(brand+36, &ebx, 4);
    memcpy(brand+40, &ecx, 4);
    memcpy(brand+44, &edx, 4);

    console_print_string("Running on a %s\n", brand);

    asm volatile ("mov %4, %%eax\ncpuid":
            "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx): "g"(1));
    uint32_t family = (eax >> 8) & 0xf;
    uint32_t model = (family == 6 || family == 0xf)
        ? ((eax >> 12) & 0xf0) + ((eax >> 4) & 0xf)
        : (eax >> 4) & 0xf ;
    uint32_t display_family = family == 0xf
        ?  ((eax >> 20) & 0xff) + family
        : family;
    console_print_string("eax %x, Model %d, family %d, stepping %d\n", eax, model, display_family, eax & 0xf);
}

void parse_memory_map() {
    struct E820 {
        char * ptr;
        uint64_t len;
        uint32_t type; // 1: usable, 2: notusable
        uint32_t ext;  //
        uint64_t pad;  // ?  not in docs
    };

    struct E820 * table = (struct E820*) 0x4000;
    kmem_add_block((char*)0x8000, 0x2000, 0x80);

    // HeapStart gives us room from 0x100000 to 0x1fffff for kernel code
    char * const HeapStart = (char*)0x200000;

    while( table->ptr || table->len ) {
        if (table->len && (table->type == 1)) {
            if (table->ptr + table->len >= HeapStart) {
                char * begin = table->ptr;
                uint64_t len = table->len;
                if (begin < HeapStart) {
                    len -= (HeapStart - begin);
                    begin = HeapStart;
                }
                console_print_string("Adding %p %x to available heap\n", begin, len);
                kmem_add_block(begin, len, 0x400);
            }
        }
        table++;
    }
}

extern void init_ata();

int main() {
    console_set_color(Green, Black);
    console_print_string("BadOS-64\n");
    console_set_color(Gray, Black);

    cpu_details();

    register_interrupt_handler(3, breakpoint, 0);
    register_interrupt_handler(0xe, page_fault, 0);
    register_interrupt_handler(0xd, protection, 0);
    register_interrupt_handler(8, double_fault, 0);

    init_gdt();
    init_interrupts();
    init_syscall();

    uint32_t ram = *(uint32_t*)0x5020;
    uint16_t cpuSpeed = *(uint16_t*)0x5010;
    console_print_string("\nSystem RAM: %d MB. CPU Speed: %d MHz\n\n",
            ram, cpuSpeed);

    kmem_init();
    parse_memory_map();

    init_pci();
    init_ne2k();

    init_keyboard();

    init_ata();

    init_http();
    init_echo();
    init_clock();

    char buf[256];
    bzero(buf, sizeof(buf));
    int r = read("MOD.TXT", buf, sizeof(buf));
    if (r < 0) {
        console_set_color(Black, Red);
        console_print_string("Cannot read mod.txt: %d\n", r);
    }
    else {
        console_set_color(Bright|Blue, Black);
        console_print_string("MOTD: %s", buf);
        console_set_color(Gray, Black);
    }
    console_set_color(Gray, Black);

    create_process(user_mode);
    console_print_string("Starting up main loop\n");

    return 0;
}

