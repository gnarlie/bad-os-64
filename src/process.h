#pragma once

#include "common.h"
#include "task.h"

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
    uint8_t db : 1;          // n.b. must be 0 in 64 bit
    uint8_t granularity : 1; // 0: bytes, 1: 4KB
    uint8_t baseHigh;
} __attribute__((packed));


struct process;
typedef void (*process_entry)();
typedef void (*process_exit)(struct process *);
typedef struct process {
    process_entry entry;
    char * stack;
    process_exit reap;
} process_t;


extern void create_tss(struct gdt_entry*);
extern process_t * create_process(void (*func)());

