[bits 64]

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

extern isr_handler

; ----- macros -----
; exceptions without error code
%macro ISR_NOERR 1
isr%1:
    cli
    push 0           ; fake err_code
    push %1          ; int_no
    jmp isr_common
%endmacro

; exceptions with error code (CPU already pushed err_code)
%macro ISR_ERR 1
isr%1:
    cli
    push %1          ; int_no (err_code is already on stack)
    jmp isr_common
%endmacro

isr_common:
    ; save general-purpose registers (must match regs_t order!)
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, rsp      ; rdi = pointer to regs_t
    call isr_handler

    ; restore registers
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16       ; drop int_no and err_code
    sti
    iretq

; ----- specific vectors -----
; no error code
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8     ; Double Fault has error code
ISR_NOERR 9
ISR_ERR   10    ; Invalid TSS
ISR_ERR   11    ; Segment Not Present
ISR_ERR   12    ; Stack-Segment Fault
ISR_ERR   13    ; General Protection
ISR_ERR   14    ; Page Fault
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17    ; Alignment Check
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30    ; Security Exception
ISR_NOERR 31

