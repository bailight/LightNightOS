global context_switch_asm

section .text

context_switch_asm:
    push    rbp
    mov     rbp, rsp
    
    test    rdi, rdi
    jz      .load_new
    mov     rax, [rdi]
    test    rax, rax
    jz      .load_new
    
    mov     [rax + 0], r15
    mov     [rax + 8], r14
    mov     [rax + 16], r13
    mov     [rax + 24], r12
    mov     [rax + 32], rbx
    mov     [rax + 40], rbp
    
    mov     rbx, [rbp + 8]
    mov     [rax + 48], rbx
    
    lea     rbx, [rbp + 16]
    mov     [rax + 72], rbx
    
    pushfq
    pop     qword [rax + 64]
    
    mov     word [rax + 56], cs
    mov     word [rax + 80], ss

.load_new:
    mov     rax, rsi
    
    mov     r15, [rax + 0]
    mov     r14, [rax + 8]
    mov     r13, [rax + 16]
    mov     r12, [rax + 24]
    mov     rbx, [rax + 32]
    mov     rbp, [rax + 40]
    
    mov     rsp, [rax + 72]
    
    push    qword [rax + 80]
    push    qword [rax + 72]
    push    qword [rax + 64]
    push    qword [rax + 56]
    push    qword [rax + 48]
    
    iretq
