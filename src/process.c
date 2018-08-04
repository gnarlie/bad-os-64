#include "process.h"

#include "memory.h"
#include "entry.h"
#include "task.h"

#define StackSize 4096

struct tss {
   uint32_t reserved0;
   uint32_t rsp0l;
   uint32_t rsp0h;
   uint32_t rsp1l;
   uint32_t rsp1h;
   uint32_t rsp2l;
   uint32_t rsp2h;
   uint32_t reserved[2];
   uint32_t ist1l;
   uint32_t ist1h;
   uint32_t ist2l;
   uint32_t ist2h;
   uint32_t ist3l;
   uint32_t ist3h;
   uint32_t ist4l;
   uint32_t ist4h;
   uint32_t ist5l;
   uint32_t ist5h;
   uint32_t ist6l;
   uint32_t ist6h;
   uint32_t ist7l;
   uint32_t ist7h;
   uint32_t reserved2[2];
   uint16_t reserved3;
   uint16_t ioMapBaseAddr;
};


static struct tss tss;

static char kernel_stack[StackSize];
static char safe_stack[StackSize];

void create_tss(struct gdt_entry* entry) {
    bzero(&tss, sizeof(tss));

    uint64_t base = (uint64_t)&tss;
    uint32_t limit = sizeof tss;

    uint64_t stack = (uint64_t)(kernel_stack + StackSize);
    tss.rsp0l = stack;
    tss.rsp0h = stack >> 32;

    // not used right now but ready for double fault, etc.
    stack = (uint64_t)(safe_stack + StackSize);
    tss.ist1l = stack;
    tss.ist1h = stack >> 32;

    entry->lowLimit = limit - 1;
    entry->baseLow = base & 0xffff;
    entry->baseMid = (base >> 16) & 0xff;
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
}

static void process_reap( process_t * proc ) {
    kmem_free(proc);
}

process_t * create_process( process_entry fn ) {
    // TODO -- map a LDT with proc->stack at a high virtual address
    process_t * proc = kmem_alloc( sizeof(struct process) + StackSize );
    proc->entry = fn;
    proc->stack = (char*)(proc + 1) + StackSize - 8;
    proc->reap = process_reap;
    task_enqueue_easy((tasklet)call_user_function, proc);

    return proc;
}

