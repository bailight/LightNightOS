OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_kstart)

PHDRS {
    text PT_LOAD FILEHDR PHDRS;
    data PT_LOAD;
    bss PT_LOAD;
}

SECTIONS
{
    . = 0x00100000;

    .text : {
        *(.text .text.*)
    } :text

    .rodata : {
        *(.rodata .rodata.*)
    } :text

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :bss

    _end = .;
}