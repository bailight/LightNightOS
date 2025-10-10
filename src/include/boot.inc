; Опрелить адрес
%define MBR_LOAD_ADDR       0x7C00
%define BOOT_LOAD_ADDR    0x8000
%define BOOT_STACK_ADDR   0x7000
%define VGA_MEM_ADDR        0xB8000
%define VGA_ROW_BYTES       160

; Определить флагов регистров
%define CR0_PE    1
%define CR0_PG    (1<<31)
%define CR4_PAE   (1<<5)
%define MSR_EFER  0xC0000080
%define EFER_LME  (1<<8)
%define KERNEL_SECTORS  64

; --- Disk Address Packet (протокол EDD) ---
DAP:
    db 0x10          ; sizeб обычно 16 бит
    db 0x00          ; reserved
    dw 128           ; количество СЕКТОРОВ для читение (каждый 512 KB)
    dw 0x8000        ; offset | Адрес смещения целевой памяти
    dw 0x0000        ; segment | Адрес целевого сегмента памяти
    dq 1             ; LBA 1 (сразу после MBR)

BOOT_DRIVE: db 0

%macro PUT32 1
    push    ax
    push    edi
    mov     edi, 0xB8000 + 160*12
    mov     ax, 0x0700 + %1
    mov     [edi], ax
    pop     edi
    pop     ax
%endmacro
