#pragma once

#include "common.h"

extern void init_syscall();
extern void syscall(void * fn, void *);

struct process;
extern void call_user_function(void (* fn), struct process*);

extern void install_gdt(void*, uint16_t);
extern void install_tss();

