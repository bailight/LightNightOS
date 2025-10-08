%include "boot.inc"
[bits 64]
global _kstart

_kstart:
    ; Пишем "OK" в 3-ю строку (row = 2)
    mov rdi, VGA_MEM_ADDR + VGA_ROW_BYTES*2
    mov word [rdi], 0x074F       ; 'O' (attr 0x07)
    add rdi, 2
    mov word [rdi], 0x074B       ; 'K'

.hang:
    hlt
    jmp .hang

