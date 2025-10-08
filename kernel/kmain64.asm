[bits 64]
global _kstart

%define VGA       0xB8000
%define ROW_BYTES 160            ; 80 колонок * 2 байта на символ

_kstart:
    ; Ничего не очищаем — сохраняем буквы из bootloader/stage2
    ; Пишем "OK" в 3-ю строку (row = 2)
    mov rdi, VGA + ROW_BYTES*2
    mov word [rdi], 0x074F       ; 'O' (attr 0x07)
    add rdi, 2
    mov word [rdi], 0x074B       ; 'K'

.hang:
    hlt
    jmp .hang

