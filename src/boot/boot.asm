%include "boot.inc"
[org BOOT_LOAD_ADDR]
[bits 16]

boot_entry:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BOOT_STACK_ADDR

    ; маяк (реальный режим 16 bit)
    mov ah, 0x0E
    mov al, 'S'
    int 0x10

    ; fast A20, чтобы были больше 1 МВ
    in   al, 0x92
    or   al, 00000010b
    out  0x92, al

    ; ===== читаем ядро BIOS-ом в 0x9000:0000 =====
    mov si, DAP_KERN
    mov ah, 0x42
    int 0x13
    jc  disk_error_kern

    mov ah, 0x0E
    mov al, 'K'
    int 0x10

    ; ----- GDT и protected mode -----
    lgdt [gdt_ptr]
    mov eax, cr0
    or  eax, CR0_PE
    mov cr0, eax
    jmp 0x08:pm32

[bits 32]
pm32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ;Инициализируем указатель на VGA
    mov dword [vga_ptr32], 0xB8000

    PUT32 'P'

    mov esi, 0x00090000
    mov edi, 0x00100000
    mov ecx, (KERNEL_SECTORS*512)/4
    rep movsd

    ; Включить PAE (расширение физического адреса)
    mov eax, cr4
    or  eax, CR4_PAE
    mov cr4, eax

    mov eax, pml4_table
    mov cr3, eax

    mov ecx, MSR_EFER
    rdmsr
    or eax, EFER_LME
    wrmsr

    mov eax, cr0
    or  eax, CR0_PG
    mov cr0, eax

    ; ===== флаг 16->32 и OK =====
    mov edi, 0xB8000 + 160*21        ; строка 22
    mov esi, msg_16_32
    call putstr32

    mov edi, 0xB8000 + 160*22        ; строка 23
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
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; верхний маркер LMO
    mov rdi, 0xB8000
    mov word [rdi], 0x074C     ; 'L'
    add rdi, 2
    mov word [rdi], 0x074D     ; 'M'
    add rdi, 2
    mov word [rdi], 0x074F     ; 'O'

    ; ===== флаги 32->64 и OK =====
    mov rdi, 0xB8000 + 160*23        ; строка 24
    mov rsi, msg_32_64
    call putstr64

    mov rdi, 0xB8000 + 160*24        ; строка 25 (последняя)
    mov rsi, msg_lm64
    call putstr64

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
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00209A0000000000
    dq 0x0000920000000000
gdt_ptr:
    dw gdt_end - gdt - 1
    dd gdt
gdt_end:

; -------- Настройка таблицы страниц 0~1 ГБ памяти)--------
align 4096
pml4_table:
    dq pdpt_table + 0x003

align 4096
pdpt_table:
    dq pd_table + 0x003
    dq 0
    dq 0
    dq 0

align 4096
pd_table:
%assign i 0
%rep 512
    dq (i*0x200000) + 0x083
%assign i i+1
%endrep

; -------- DAP ядра --------
DAP_KERN:
    db 0x10
    db 0x00
    dw KERNEL_SECTORS
    dw 0x0000
    dw 0x9000
    dq 100

disk_error_kern:
    mov ah, 0x0E
    mov al, 'X'
    int 0x10
.hang:
    cli
    hlt
    jmp .hang

vga_ptr32: dd 0

; -------- строки-маячки --------
msg_16_32: db "[16->32]", 0
msg_pm32:  db "PM32 OK", 0
msg_32_64: db "[32->64]", 0
msg_lm64:  db "LM64 OK", 0

