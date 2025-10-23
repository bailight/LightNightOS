%include "boot.inc"
[org BOOT_LOAD_ADDR]
[bits 16]

boot_entry:
    ; Инициализировать сегментные регистры и стек
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BOOT_STACK_ADDR

    ; Маяк 'S' (реальный режим 16 bit)
    mov ah, 0x0E
    mov al, 'S'
    int 0x10

    ; Включить Fast A20 для доступа к памяти свыше 1 МБ
    in   al, 0x92
    or   al, 00000010b
    out  0x92, al

    ; Прочитать ядро BIOS-ом ​​до 0x9000:0000
    mov si, DAP_KERN
    mov ah, 0x42
    int 0x13
    jc  disk_error_kern

    ; Маяк 'K'
    mov ah, 0x0E
    mov al, 'K'
    int 0x10

    ; Загрузка GDT и входить в защищенный режим
    lgdt [gdt_ptr]
    mov eax, cr0
    or  eax, CR0_PE
    mov cr0, eax
    jmp 0x08:pm32

[bits 32]
pm32:
    ; Инициализировать 32-битные сегментные регистры
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Инициализировать указатель на VGA
    mov dword [vga_ptr32], 0xB8000

    ; Копировать ядро ​​из 0x90000 в 0x100000
    mov esi, 0x00090000
    mov edi, 0x00100000
    mov ecx, (KERNEL_SECTORS*512)/4
    rep movsd

    ; Включить PAE (расширение физического адреса)
    mov eax, cr4
    or  eax, CR4_PAE
    mov cr4, eax

    ; Настроить таблицу страниц
    mov eax, pml4_table
    mov cr3, eax

    ; Включить длинный режим
    mov ecx, MSR_EFER
    rdmsr
    or eax, EFER_LME
    wrmsr

    ; Включить страниц и входить в длинный режим
    mov eax, cr0
    or  eax, CR0_PG
    mov cr0, eax

    ; ===== флаг 16->32 и OK =====
    mov edi, VGA_MEM_ADDR + VGA_ROW_BYTES*21        ; строка 22
    mov esi, msg_16_32
    call putstr32

    mov edi, VGA_MEM_ADDR + VGA_ROW_BYTES*22        ; строка 23
    mov esi, msg_pm32
    call putstr32

    jmp 0x18:lm64

; ---- функция вывода строки в 32-бит VGA ----
putstr32:
    push eax
    push edi
    push esi
.put_loop32:
    lodsb
    test al, al
    jz .done32
    mov ah, 0x07
    stosw
    jmp .put_loop32
.done32:
    pop esi
    pop edi
    pop eax
    ret

[bits 64]
lm64:
    ; Инициализация 64-битных сегментных регистров
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; ===== флаги 32->64 и OK =====
    mov rdi, VGA_MEM_ADDR + VGA_ROW_BYTES*23        ; строка 24
    mov rsi, msg_32_64
    call putstr64

    mov rdi, VGA_MEM_ADDR + VGA_ROW_BYTES*24        ; строка 25 (последняя)
    mov rsi, msg_lm64
    call putstr64

    ; Перейти к ядру
    mov rax, 0x00100000
    jmp rax

; ---- функция вывода строки в 64-бит VGA ----
putstr64:
    push rax
    push rdi
    push rsi
.put_loop64:
    lodsb
    test al, al
    jz .done64
    movzx eax, al
    or ax, 0x0700
    stosw
    jmp .put_loop64
.done64:
    pop rsi
    pop rdi
    pop rax
    ret

; ---------------- GDT ----------------
align 8
gdt:
    dq 0x0000000000000000       ; Пустой дескриптор
    dq 0x00CF9A000000FFFF       ; Дескриптор сегмента кода (32 бита)
    dq 0x00CF92000000FFFF       ; Дескриптор сегмента данных (32 бита)
    dq 0x00209A0000000000       ; Дескриптор сегмента кода (64 бита)
    dq 0x0000920000000000       ; Дескриптор сегмента данных (64 бита)
gdt_ptr:
    dw gdt_end - gdt - 1        ; Лимит GDT
    dd gdt                      ; Базовый адрес GDT
gdt_end:

; -------- Настройка таблицы страниц 0~1 ГБ памяти)--------
align 4096
pml4_table:
    dq pdpt_table + 0x003

align 4096
pdpt_table:
    dq pd_table + 0x003
    times 3 dq 0

align 4096
pd_table:
%assign i 0
%rep 512
    dq (i*0x200000) + 0x083
%assign i i+1
%endrep

disk_error_kern:
    mov ah, 0x0E
    mov al, 'X'
    int 0x10
.hang:
    cli
    hlt
    jmp .hang

vga_ptr32: dd 0

DAP_KERN:
    db 0x10
    db 0x00
    dw KERNEL_SECTORS
    dw 0x0000
    dw 0x9000
    dq 100

; -------- строки-маячки --------
msg_16_32: db "[16->32]", 0
msg_pm32:  db "PM32 OK", 0
msg_32_64: db "[32->64]", 0
msg_lm64:  db "LM64 OK", 0

