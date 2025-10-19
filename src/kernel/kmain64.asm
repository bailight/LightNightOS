[bits 64]
global _kstart
extern kernel_init

_kstart:

    jmp kernel_init

.hang:
    hlt
    jmp .hang

