OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_kstart)

SECTIONS
{
  . = 0x00100000;

  .text : {
    *(.text .text.*)
  }

  .rodata : {
    *(.rodata .rodata.*)
  }

  .data : {
    *(.data .data.*)
  }

  .bss : {
    *(COMMON)
    *(.bss .bss.*)
  }
}

