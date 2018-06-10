USE64


[EXTERN main]
[EXTERN irq_handler]
[EXTERN isr_handler]
[EXTERN console_clear_screen]
[EXTERN console_print_string]
[EXTERN task_poll_for_work]
[EXTERN panic]

[GLOBAL start]
[GLOBAL create_gate]
[GLOBAL create_isr_handler]
[GLOBAL install_gdt]
[GLOBAL install_tss]
[GLOBAL call_user_function]
[GLOBAL syscall]
[GLOBAL init_syscall]


start:
    mov rsp, 0x18000            ; move this elsewhere
    call console_clear_screen
    mov rdi, hello_message
    call console_print_string
    call main
    mov [lastStack], esp
main_loop:
    mov eax, [lastStack]
    cmp eax, esp
    jne stack_slam
    call task_poll_for_work
    sti
    hlt
    jmp main_loop

stack_slam:
    mov rdi, stack_differs
    call console_print_string
    mov esp, [lastStack]
    jmp main_loop

%define segment(idx, dpl) ((idx << 3) + dpl)

%define kernel_code segment(1,0)
%define kernel_data segment(2,0)
%define user_32_code segment(3,3)
%define user_code segment(5,3)
%define user_data segment(4,3)

; rdi - data to be copied
; esi - size of data in bytes
install_gdt:
    mov ecx, esi
    dec esi
    mov word [gdtr64], si
    mov rsi, rdi
    mov rdi, 0x00001000      ; GDT address
    mov [gdtr64+2], rdi     ; stor in GDTR
    rep movsb               ; copy to here
    lgdt [gdtr64]

    mov ax, kernel_data
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; nasty bit to reload CS
    mov rax, reload_done
    push kernel_code
    push rax
    o64 retf
reload_done:
    ret

install_tss:
    mov ax, segment(6,0)
    ltr ax
    ret

; rdi - user function to call
call_user_function:
    ; switch data selectors to user mode selectors
    mov ax, user_data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rax, rsp

    ; fake interrupt frame
    push user_data
    push rax           ; stack
    pushf

    push user_code
    push user_entry
    iretq

user_entry:
    call rdi

    ; ring 3 thunk to get back to kernel space
    mov rdi, end_user_fn
    syscall

; back in kernel space for good
end_user_fn:

    ; Unwind the stack to where we were before calling user space
    pop rax     ; w/b next RIP if we were to return
    pop rax     ; w/b next RIP if the sysret would happen
    pop rax     ; w/b flags if sysret would happen

    ret  ; next  RIP effectively returns from call_user_function

; *********************************************
; System call crappe

init_syscall:
    ; bits 16..31: sysret 32 bit cs
    ;   cs64 is cs32 + 16
    ;   ss is cs32 + 8
    ; bits  0..15: syscall cs
    ;   ss is cs + 8
    mov edx, (user_32_code << 16) + kernel_code
    xor eax, eax
    mov ecx, 0xc0000081 ; STAR
    wrmsr

    mov rax, syscall_enter
    mov rdx, rax
    shr rdx, 32
    mov ecx, 0xc0000082 ; LSTAR
    wrmsr

    ret

; rdi - system call function
; rsi - only parameter
syscall:
    syscall
    ret

syscall_enter:
    ; here be kernel mode again
    mov ax, ss
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push r11 ; store adjusted flags
    push rcx ; store next rip
             ; should switch stacks here as well

    mov rax, rdi
    mov rdi, rsi
    call rax

    pop rcx ; pop next ring 3 instruction
    pop r11 ; pop flags

    ; get ready for user again
    mov ax, user_data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    o64 sysret ; sysretq

; ----------------------
; Create a call gate for an irq. Stores the IDT entry at ES:[EDI*16]. Assumes that the IDTR
; has been set to point to address 0. Pure64 has done this for us. TODO: setup our own dammnit!
;   rdi - gate number
;   rsi - function to call
create_gate:
    shl rdi, 4
    mov rax, rsi

; IDT entry
;   2 bytes : low bits of fun
;   2 byte  : cs selector
;   1 byte  : 0
;   1 byte  : flags
;   2 bytes : middle 16 bits
;   4 bytes : high 32 bits
;   4 bytes : 0

    stosw               ; store the low word (15..0)
    mov ax, kernel_code ; code selector
    stosw
    xor ax, ax          ; 0
    stosb
    mov al,  0x8e       ; flags: present: 1, DPL: 3, storage: 0, type: interrupt
    stosb
    shr rax, 16         ; move mid word to ax & store
    stosw
    shr rax, 16         ; move high dword to ax & store
    stosd
    xor rax, rax
    stosd
    ret

%macro PUSH_ALL 0

    push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdx
    push rcx
    push rbx
    push rax
    push rsi
    push rdi

    mov ecx, gs
    push rcx
    mov ecx, fs
    push rcx
    mov ecx, es
    push rcx
    mov ecx, ds
    push rcx
    mov ecx, ss
    push rcx

    mov rsi, rsp
%endmacro

%macro POP_ALL 0
    pop rcx
    ; mov ss, ecx
    pop rcx
    mov ds, ecx
    pop rcx
    mov es, ecx
    pop rcx
    mov fs, ecx
    pop rcx
    mov gs, ecx

    pop rdi
    pop rsi
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp

    add rsp, 8
%endmacro

%macro ISR_ERR 1
    [GLOBAL isr%1]
    isr%1:
        cli
        PUSH_ALL
        mov edi, dword %1
        jmp isr_common
%endmacro

%macro ISR 1
    [GLOBAL isr%1]
    isr%1:
        cli
        push 0
        PUSH_ALL
        mov edi, dword %1
        jmp isr_common
%endmacro

isr_common:
   call isr_handler
   POP_ALL
   sti
   iretq

%macro IRQ 2
    [GLOBAL irq%1]
    irq%1:
        cli
        push 0
        PUSH_ALL
        mov edi, dword %2
        jmp irq_common
%endmacro

irq_common:
   mov al, 0x20
   cmp edi, 39
   jle reset_master
   out 0xa0, al ; reset slave
reset_master:
   out 0x20, al ; reset master
   call irq_handler
   POP_ALL
   sti
   iretq

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

ISR 0
ISR 1
ISR 2
ISR 3
ISR 4
ISR 5
ISR 6
ISR 7
ISR_ERR 8
ISR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR 15
ISR 16
ISR_ERR 17
ISR 18
ISR 19
ISR 20
ISR 21
ISR 22
ISR 23
ISR 24
ISR 25
ISR 26
ISR 27
ISR 28
ISR 29
ISR 30
ISR 31

lastStack     dq 0
stack_differs db `Stack pointer has changed\n`, 0
hello_message db `Initializing BadOS-64\n`, 0
pct_p         db `here: %p\n`, 0
gdtr64        dw 0
              dq 0x0000000000001000
