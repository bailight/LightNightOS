%define CR0_PE    1
%define CR0_PG    (1<<31)
%define CR4_PAE   (1<<5)
%define MSR_EFER  0xC0000080
%define EFER_LME  (1<<8)

%define KERNEL_SECTORS  64
%define E820_BUF        0x7E00
%define E820_MAX_ENTRIES 32

e820_count: dd 0

[org 0x8000]
[bits 16]

boot_entry:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000

    ; beacon (realtime mode)
    mov ah, 0x0E
    mov al, 'S'
    int 0x10

    call read_e820

    ; fast A20
    in   al, 0x92
    or   al, 00000010b
    out  0x92, al

    ; reading kernel using BIOS in 0x9000:0000
    call disk_read_kernel

    mov ah, 0x0E
    mov al, 'K'
    int 0x10

    ; ----- GDT & protected mode -----
    lgdt [gdt_ptr]
    mov eax, cr0
    or  eax, CR0_PE
    mov cr0, eax

    jmp 0x08:pm32

read_e820:
    pusha
    mov di, E820_BUF
    xor ebx, ebx
    mov edx, 0x534D4150

.read_loop:
    mov eax, 0xE820
    mov ecx, 24
    mov dword [es:di + 20], 0
    int 0x15

    jc .done
    cmp eax, 0x534D4150
    jne .done

    inc dword [e820_count]
    add di, 20

    mov eax, [e820_count]
    cmp eax, E820_MAX_ENTRIES
    jge .done

    test ebx, ebx
    jnz .read_loop

.done:
    ; Memory layout read successful
    mov ah, 0x0E
    mov al, 'M'
    int 0x10

    popa
    ret

disk_read_kernel:
    pusha
    mov si, DAP_KERN
    mov ah, 0x42
    int 0x13

    ; CF=1 - error with read disk
    jc disk_error_kern

    popa
    ret

disk_error_kern:
    mov ah, 0x0E
    mov al, 'X'
    int 0x10

DAP_KERN:
    db 0x10
    db 0x00
    dw KERNEL_SECTORS
    dw 0x0000
    dw 0x9000
    dq 100

; ==============================================
; change mode：realtime mode → protect mode
; ==============================================

[bits 32]
pm32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov dword [vga_ptr32], 0xB8000

    mov esi, 0x00090000
    mov edi, 0x00100000
    mov ecx, (KERNEL_SECTORS*512)/4
    rep movsd

    mov esi, e820_count
    mov edi, 0x00110000
    mov ecx, 1
    rep movsd
    
    mov esi, E820_BUF
    mov ecx, [e820_count]
    imul ecx, ecx, 5
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

    ; ===== print flag 16->32 and OK =====
    mov edi, 0xB8000 + 160*21        ; line 22
    mov esi, msg_16_32
    call putstr32

    mov edi, 0xB8000 + 160*22        ; line 23
    mov esi, msg_pm32
    call putstr32

    jmp 0x18:lm64

; function of printing line in 32-bits VGA
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

; ==============================================
; change mode: protect mode -> long mode
; ==============================================

[bits 64]
lm64:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; ===== printing flags 32->64 and OK =====
    mov rdi, 0xB8000 + 160*23
    mov rsi, msg_32_64
    call putstr64

    mov rdi, 0xB8000 + 160*24
    mov rsi, msg_lm64
    call putstr64

    ; mov rdi, E820_BUF
    ; mov rsi, [e820_count]

    mov rax, 0x00100000
    jmp rax

; function of printing line in 64-bits VGA
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

; ==============================================
; Global Descriptor Table (GDT)
; ==============================================

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

; ==============================================
; Page table initialization
; ==============================================

align 4096
pml4_table:
    dq pdpt_table + 0x003
    times 255 dq 0
    dq pdpt_table_high + 0x003
    times 254 dq 0


align 4096
pdpt_table:
    dq pd_table + 0x003
    dq 0
    dq 0
    dq 0

align 4096
pdpt_table_high:
    dq pd_table_high + 0x003
    times 511 dq 0

align 4096
pd_table:
%assign i 0
%rep 512
    dq (i*0x200000) + 0x083
%assign i i+1
%endrep

align 4096  
pd_table_high:
%assign i 0
%rep 512
    dq (i * 0x200000) + 0x083
%assign i i+1
%endrep

.hang:
    cli
    hlt
    jmp .hang

vga_ptr32: dd 0

msg_16_32: db "[16->32]", 0
msg_pm32:  db "PM32 OK", 0
msg_32_64: db "[32->64]", 0
msg_lm64:  db "LM64 OK", 0