USE64
;ORG 0x100000

[EXTERN main]
[GLOBAL clear_screen]
[GLOBAL print_to_console]
[GLOBAL start]

start:
    call clear_screen
say_hi:
    mov rdi, hello_message
    call print_to_console
    mov rdi, hello_message
    call print_to_console
    call main
done:
    jmp $

print_to_console:
    mov eax, 0xB8000
    mov ebx, [cursor_x]
    add eax, ebx
    add eax, ebx
    mov ebx, [cursor_y]
    imul ebx, 160
    add eax, ebx
    xor ebx, ebx
p2c_loop:
    mov edx, [rdi]
    test dl, dl
    je p2c_done
    cmp dl, 10
    je p2c_nl
    mov [rax], dl
    add rax, 2
    add rdi, 1
    add ebx, 1
    jmp p2c_loop
p2c_nl:
    mov ebx, [cursor_y]
    inc ebx
    mov [cursor_y], ebx
    imul ebx, 160
    mov eax, 0xB8000
    add eax, ebx
    xor ebx, ebx
    mov [cursor_x], ebx
    add rdi, 1
    jmp p2c_loop
p2c_done:
    add ebx, [cursor_x]
    mov [cursor_x], ebx
    ret

clear_screen:
    mov eax, 0xB8000
    mov ecx, eax
    add ecx, 4000
    mov dx, 0x0f20
clear_screen_loop:
    test ecx, eax
    je clear_screen_done
    mov [eax], dx
    add eax, 2
    jmp clear_screen_loop
clear_screen_done:
    mov byte[cursor_x], 0
    mov byte[cursor_y], 0
    ret

cursor_x dd 0x0
cursor_y dd 0x0

hello_message db `Hello, World\n`, 0
