global context_switch_asm

section .text

; void context_switch_asm(process_context_t *old, process_context_t *new)
; old может быть NULL при первом запуске
context_switch_asm:
    ; rdi = old context (может быть NULL)
    ; rsi = new context (всегда валиден)

    ; Сохраняем старый контекст (если есть)
    test    rdi, rdi
    jz      .load_new

    ; Сохраняем callee-saved регистры
    mov     [rdi + 0],  rbx
    mov     [rdi + 8],  rbp
    mov     [rdi + 16], r12
    mov     [rdi + 24], r13
    mov     [rdi + 32], r14
    mov     [rdi + 40], r15
    
    ; Сохраняем RSP (текущий стек)
    mov     [rdi + 48], rsp
    
    ; Сохраняем адрес возврата (RIP)
    mov     rax, [rsp]
    mov     [rdi + 56], rax

.load_new:
    ; Загружаем новый контекст
    mov     rbx, [rsi + 0]
    mov     rbp, [rsi + 8]
    mov     r12, [rsi + 16]
    mov     r13, [rsi + 24]
    mov     r14, [rsi + 32]
    mov     r15, [rsi + 40]
    mov     rsp, [rsi + 48]
    
    ; Загружаем RIP и прыгаем туда
    mov     rax, [rsi + 56]
    jmp     rax
