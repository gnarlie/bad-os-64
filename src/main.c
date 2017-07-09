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

#include "fs/vfs.h"

#include "service/http.h"
#include "service/echo.h"
#include "service/clock.h"


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
}

static void protection(registers_t* regs, void*user) {
    if (user)
        console_print_string("gpf\n");
    else
        console_print_string("pf\n");
    dump_regs(regs);
    panic("cannot continue");
}

struct gdt_entry {
    uint16_t lowLimit;
    uint16_t baseLow;
    uint8_t baseMid;
    uint8_t type : 4;        // segment type
    uint8_t descType : 1;    // 0 system, 1 code/data
    uint8_t privLvl : 2;
    uint8_t present : 1;
    uint8_t segLimit : 4;
    uint8_t available : 1;
    uint8_t l : 1;           // 64 bit segment
    uint8_t db : 1;          // lots of crappe
    uint8_t granularity : 1; // 0: bytes, 1 4KB
    uint8_t baseHigh;
} __attribute__((packed));

struct tss {
   uint32_t reserved0;
   uint64_t rsp0;
   uint64_t rsp1;
   uint64_t rsp2;
   uint64_t reserved1;
   uint64_t ist1;
   uint64_t ist2;
   uint64_t ist3;
   uint64_t ist4;
   uint64_t ist5;
   uint64_t ist6;
   uint64_t ist7;
   uint64_t reserved2;
   uint16_t reserved3;
   uint16_t ioMapBaseAddr;
};

static struct tss tss;

static void create_tss(struct gdt_entry* entry) {
    bzero(&tss, sizeof(tss));

    uint64_t base = (uint64_t)&tss;
    uint32_t limit = sizeof tss;

    entry->lowLimit = limit - 1;
    entry->baseLow = base & 0xffff;
    entry->baseMid = (base >> 8) & 0xff;
    entry->type = 9;
    entry->descType = 0;
    entry->privLvl = 3;
    entry->present = 1;
    entry->segLimit = 0;
    entry->available = 0;
    entry->l = 0;
    entry->db = 0;
    entry->granularity = 0;
    entry->baseHigh = (base >> 24) & 0xff;

    // TODO IST's
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
    gdt[1].granularity = 1;
    gdt[1].l = 1;
    gdt[1].present = 1;
    gdt[1].descType = 1;
    gdt[1].type = 0xA; // execute / read
    gdt[1].lowLimit = 0xffff;
    gdt[1].segLimit = 0xf;

    // gdt[2] is kernel data
    gdt[2] = gdt[1];
    gdt[2].type = 6;  // read / write

    // gdt[3] is another null selector (s/b 32 bit code)
    gdt[3] = gdt[0];

    // gdt[4] is user data
    gdt[4] = gdt[2];
    gdt[4].privLvl = 3;

    // gdt[5] is user code (64 bit)
    gdt[5] = gdt[1];
    gdt[5].privLvl = 3;

    // gdt[6/7] is the 64bit TSS descriptor
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

extern void init_ata();

#include "fs/vfs.h"

void main() {
    console_set_color(Green, Black);
    console_print_string("BadOS-64\n");
    console_set_color(Gray, Black);

    cpu_details();

    init_interrupts();
    init_gdt();
    init_syscall();

    kmem_init();

    uint32_t ram = *(uint32_t*)0x5020;
    uint16_t cpuSpeed = *(uint16_t*)0x5010;
    console_print_string("\nSystem RAM: %d MB. CPU Speed: %d MHz\n\n",
            ram, cpuSpeed);

    // TODO need to make these less arbiratry, based on the
    // actual memory map
    kmem_add_block(0x200000, 1024*1024*1024, 0x400);

    init_pci();
    init_ne2k();

    init_keyboard();

    register_interrupt_handler(3, breakpoint, 0);
    register_interrupt_handler(0xd, protection, 0);
    register_interrupt_handler(0xe, protection, (void*)1);
    register_interrupt_handler(8, double_fault, 0);

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
        console_set_color(Gray, Black);
    }
    else {
        console_set_color(Bright|Blue, Black);
        console_print_string("MOTD: %s", buf);
        console_set_color(Gray, Black);
    }

    call_user_function(user_mode);
}

