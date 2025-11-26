[bits 64]
global update_cursor
global _kstart
global lidt
global sti
global keyboard_handler
extern keyboard_handler_c
extern kernel_init

VGA_WIDTH    equ 80
VGA_HEIGHT   equ 25
VGA_MEMORY   equ 0xB8000
VGA_CTRL_PORT equ 0x3D4
VGA_DATA_PORT equ 0x3D5

section .bss
align 16
stack_bottom:
resb 16384
stack_top:

section .text

_kstart:
    mov rsp, stack_top

    jmp kernel_init

lidt:
    lidt [rdi]
    ret

sti:
    sti
    ret

keyboard_handler:
    swapgs
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    call keyboard_handler_c

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    iretq

.hang:
    hlt
    jmp .hang