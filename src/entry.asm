USE64

[EXTERN main]
[EXTERN irq_handler]
[EXTERN isr_handler]
[EXTERN console_clear_screen]
[EXTERN console_print_string]

[GLOBAL start]
[GLOBAL create_gate]
[GLOBAL create_isr_handler]


start:
    sti
    call console_clear_screen
say_hi:
    mov rdi, hello_message
    call console_print_string
    call main
done:
;    inc byte [count]
;    mov ax,[count]
;    mov [0xB8000], al
    hlt
    jmp done

create_gate:
    mov rax, rsi
; IDT entry
;   2 bytes : low bits of fun
;   2 byte : cs selector
;   1 byte : 0
;   1 byte : flags
;   2 bytes : middle 16 bits
;   4 bytes : high 32 bits
;   4 bytes : 0

	shl rdi, 4			; quickly multiply rdi by 16
	stosw				; store the low word (15..0)
	shr rax, 16
	add rdi, 4			; skip the gate marker
	stosw				; store the high word (31..16)
	shr rax, 16
	stosd				; store the high dword (63..32)
ret

%macro ISR 1
    [GLOBAL isr%1]
    isr%1:
        cli
        push rdi
        mov edi, dword %1
        jmp isr_common
%endmacro

isr_common:
   call isr_handler
   sti
   pop rdi
   iretq

%macro IRQ 2
    [GLOBAL irq%1]
    irq%1:
        cli
        push rdi
        mov edi, dword %2
        jmp irq_common
%endmacro

irq_common:
   push rax
   cmp edi, 39
   mov al, 0x20
   jle reset_master
   out 0xa0, al ; reset slave
reset_master:
   out 0x20, al ; reset master
   call irq_handler
   sti
   pop rax
   pop rdi
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
ISR 8
ISR 9
ISR 10
ISR 11
ISR 12
ISR 13
ISR 14
ISR 15
ISR 16
ISR 17
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

count dd 0x0

hello_message db `Hello, World\n`, 0
