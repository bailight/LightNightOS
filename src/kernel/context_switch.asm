global context_switch_asm

section .text

context_switch_asm:
    cmp     rdi, 0
    je      .load_new_context
    
    mov     rax, [rdi]
    
    mov     [rax + 0], r15
    mov     [rax + 8], r14
    mov     [rax + 16], r13
    mov     [rax + 24], r12
    mov     [rax + 32], r11
    mov     [rax + 40], r10
    mov     [rax + 48], r9
    mov     [rax + 56], r8
    mov     [rax + 64], rdi
    mov     [rax + 72], rsi
    mov     [rax + 80], rbp
    mov     [rax + 88], rbx
    mov     [rax + 96], rdx
    mov     [rax + 104], rcx
    mov     [rax + 112], rax
    
    mov     rbx, [rsp]
    mov     [rax + 152], rbx
    
    pushfq
    pop     qword [rax + 168]
    
    mov     [rax + 176], rsp
    
    mov     word [rax + 160], 0x08
    mov     word [rax + 184], 0x10

.load_new_context:
    mov     rax, rsi
    
    mov     r15, [rax + 0]
    mov     r14, [rax + 8]
    mov     r13, [rax + 16]
    mov     r12, [rax + 24]
    mov     r11, [rax + 32]
    mov     r10, [rax + 40]
    mov     r9,  [rax + 48]
    mov     r8,  [rax + 56]
    mov     rdi, [rax + 64]
    
    mov     rbx, [rax + 88]
    push    rbx
    
    mov     rdx, [rax + 96]
    mov     rcx, [rax + 104]
    
    mov     rsp, [rax + 176]
    
    push    qword [rax + 184]
    push    qword rsp
    push    qword [rax + 168]
    push    qword [rax + 160]
    push    qword [rax + 152]
    
    mov     rsi, [rax + 72]
    mov     rbp, [rax + 80]
    mov     rax, [rax + 112]
    
    pop     rbx
    
    iretq
