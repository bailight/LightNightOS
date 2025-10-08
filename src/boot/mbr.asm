%include "boot.inc"
[org MBR_LOAD_ADDR]
[bits 16]

start:
    ; инизация регистры
    cli
    xor ax, ax
    mov ds, ax        ; ВАЖНО: DS=0, чтобы DS:SI на DAP был физ. 0x7Cxx
    mov es, ax
    mov ss, ax
    mov sp, MBR_LOAD_ADDR
    sti

    ; сохранение номера старта диск
    mov [BOOT_DRIVE], dl

    ; маяк: 'A'
    mov ah, 0x0E
    mov al, 'A'
    int 0x10

    mov si, DAP
    mov dl, [BOOT_DRIVE]
    mov ah, 0x42
    int 0x13
    jc disk_error

    ; маяк: 'R'
    mov ah,0x0E
    mov al,'R'
    int 0x10

    jmp 0x0000:0x8000     ; прыгать на части boot

disk_error:
    mov ah,0x0E
    mov al,'E'
    int 0x10
    cli
    hlt
    jmp $

times 510-($-$$) db 0
dw 0xAA55
