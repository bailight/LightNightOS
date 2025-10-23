[bits 64]
global update_cursor
global _kstart
extern kernel_init

VGA_WIDTH    equ 80
VGA_HEIGHT   equ 25
VGA_MEMORY   equ 0xB8000
VGA_CTRL_PORT equ 0x3D4
VGA_DATA_PORT equ 0x3D5

_kstart:
    mov rdi, 0
    mov rsi, 0
    call update_cursor

    jmp kernel_init

update_cursor:
    ; 1. 计算光标在VGA缓冲区的绝对位置：row * 80 + col
    mov rax, rdi        ; rax = 行号
    mov rbx, VGA_WIDTH  ; rbx = 每行字符数（80）
    mul rbx             ; rax = 行号 * 80（无符号乘法，结果存rax）
    add rax, rsi        ; rax = 最终光标位置（0~1999，覆盖25*80=2000个位置）
    mov rcx, rax        ; 暂存光标位置到rcx（避免后续操作覆盖rax）

    ; 2. 发送命令：设置光标高位字节（命令码0x0E）
    mov dx, VGA_CTRL_PORT  ; dx = 控制端口（64位out指令要求端口在dx）
    mov al, 0x0E           ; al = 命令码（设置光标高位）
    out dx, al             ; 发送命令到控制端口

    ; 3. 发送光标高位数据（rax的高8位）
    mov dx, VGA_DATA_PORT  ; dx = 数据端口
    mov al, ch             ; al = 光标位置的高8位（rcx的高8位，即ch）
    out dx, al             ; 发送高位数据

    ; 4. 发送命令：设置光标低位字节（命令码0x0F）
    mov dx, VGA_CTRL_PORT
    mov al, 0x0F           ; al = 命令码（设置光标低位）
    out dx, al

    ; 5. 发送光标低位数据（rax的低8位）
    mov dx, VGA_DATA_PORT
    mov al, cl             ; al = 光标位置的低8位（rcx的低8位，即cl）
    out dx, al

    ret  ; 函数返回（回到C代码）

; 异常停机（备用）
.hang:
    hlt
    jmp .hang