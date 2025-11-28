OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_kstart)

PHDRS {
    text PT_LOAD FILEHDR PHDRS FLAGS(5);
    data PT_LOAD FLAGS(6);
    bss PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0x00100000;

    .text : {
        __start_code = .;
        *(.text .text.*)
        __end_code = .;
    } :text

    .rodata : {
        *(.rodata .rodata.*)
    } :text

    .data : {
        *(.data .data.*)
        __end_data = .;
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
        __end_brk = .;
    } :bss

    _end = .;
    . = ALIGN(2*1024*1024);
    __end_align = .;
}