[bits 64]

global _kstart
global lidt
global sti

global keyboard_handler
global timer_handler

extern kernel_init
extern keyboard_handler_c
extern timer_handler_c

section .text


_kstart:
    mov rsp, 0x90000
    call kernel_init

.hang:
    hlt
    jmp .hang


lidt:
    lidt [rdi]
    ret

; void sti(void)
sti:
    sti
    ret


%macro PUSH_ALL 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POP_ALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

; IRQ1
keyboard_handler:
    PUSH_ALL
    call keyboard_handler_c
    POP_ALL
    iretq

; IRQ0
timer_handler:
    PUSH_ALL
    call timer_handler_c
    POP_ALL
    iretq

