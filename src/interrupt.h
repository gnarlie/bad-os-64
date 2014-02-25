#pragma once

#include "common.h"

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

void init_interrupts();
void register_interrupt_handler(uint8_t interrupt, isr_t);

void disable_interrupts();
void enable_interrupts();
