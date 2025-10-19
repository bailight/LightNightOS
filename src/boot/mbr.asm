%include "boot.inc"
[org MBR_LOAD_ADDR]
[bits 16]

start:
    ; Отключить прерывания, инициализировать сегментные регистры и указатели стека
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, MBR_LOAD_ADDR
    sti

    mov ax, 0x03
    int 0x10

    ; сохранение номера старта диск
    mov [BOOT_DRIVE], dl

    ; маяк 'A'
    mov ah, 0x0E
    mov al, 'A'
    int 0x10

    ; Подготовить структуру DAP и прочитать последующие сектора
    mov si, DAP
    mov dl, [BOOT_DRIVE]
    mov ah, 0x42
    int 0x13
    jc disk_error

    ; маяк 'R'
    mov ah, 0x0E
    mov al, 'R'
    int 0x10

    ; прыгать на части boot
    jmp 0x0000:BOOT_LOAD_ADDR

disk_error:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10
    cli
    hlt
    jmp $

; Заполнить до 510 байт и добавить флаг запуска
times 510-($-$$) db 0
dw 0xAA55