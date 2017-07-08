extern void init_syscall();
extern void syscall(void * fn, void *);
extern void call_user_function(void*);

extern void install_gdt(void*, uint16_t);
extern void install_tss();


