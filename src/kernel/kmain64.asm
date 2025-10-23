[bits 64]
global update_cursor
global _kstart
global lidt
global sti
global timer_handler
extern kernel_init
extern timer_handler_c

VGA_WIDTH    equ 80
VGA_HEIGHT   equ 25
VGA_MEMORY   equ 0xB8000
VGA_CTRL_PORT equ 0x3D4
VGA_DATA_PORT equ 0x3D5

_kstart:
    mov rdi, 0
    mov rsi, 0
    call update_cursor

    jmp kernel_init

update_cursor:
    mov rax, rdi
    mov rbx, VGA_WIDTH
    mul rbx
    add rax, rsi
    mov rcx, rax

    mov dx, VGA_CTRL_PORT
    mov al, 0x0E
    out dx, al

    mov dx, VGA_DATA_PORT
    mov al, ch 
    out dx, al

    mov dx, VGA_CTRL_PORT
    mov al, 0x0F
    out dx, al

    mov dx, VGA_DATA_PORT
    mov al, cl
    out dx, al

    ret

lidt:
    lidt [rdi]
    ret

sti:
    sti
    ret

timer_handler:
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

    call timer_handler_c

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