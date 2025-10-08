; boot/stage2_long.asm
[org 0x8000]
[bits 16]

%define CR0_PE    1
%define CR0_PG    (1<<31)
%define CR4_PAE   (1<<5)
%define MSR_EFER  0xC0000080
%define EFER_LME  (1<<8)

%define KERNEL_SECTORS  64

stage2_entry:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000

    ; маяк (реальный режим)
    mov ah, 0x0E
    mov al, 'S'
    int 0x10

    ; fast A20
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

; =====================================================================
[bits 32]
pm32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov dword [vga_ptr32], 0xB8000

%macro PUT32 1
    mov edi, [vga_ptr32]
    mov ax, 0x0700 + %1
    mov [edi], ax
    add edi, 2
    mov [vga_ptr32], edi
%endmacro

    PUT32 'P'

    mov esi, 0x00090000
    mov edi, 0x00100000
    mov ecx, (KERNEL_SECTORS*512)/4
    rep movsd

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

    ; ===== выводим флаг 16->32 и OK =====
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

; =====================================================================
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

    ; ===== выводим флаги 32->64 и OK =====
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

; =====================================================================
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

; =====================================================================
; -------- identity map 0..1GiB --------
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

; =====================================================================
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

; =====================================================================
; -------- строки-маячки --------
msg_16_32: db "[16->32]", 0
msg_pm32:  db "PM32 OK", 0
msg_32_64: db "[32->64]", 0
msg_lm64:  db "LM64 OK", 0

