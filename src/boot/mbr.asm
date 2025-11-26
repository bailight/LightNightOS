[org 0x7C00]
[bits 16]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [BOOT_DRIVE], dl

    ; маяк: 'A'
    mov ah,0x0E
    mov al,'A'
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

    jmp 0x0000:0x8000     ; jumping to boot.asm

disk_error:
    mov ah,0x0E
    mov al,'E'
    int 0x10
    cli
    hlt
    jmp $

; --- Disk Address Packet (EDD) ---
DAP:
    db 0x10          ; size
    db 0x00          ; reserved
    dw 128           ; sectors to read
    dw 0x8000        ; offset
    dw 0x0000        ; segment
    dq 1             ; LBA 1 (immediately after MBR)

BOOT_DRIVE: db 0

times 510-($-$$) db 0
dw 0xAA55