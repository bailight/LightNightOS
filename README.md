# LightNightOS
OS project 【Tazieva Kamilla & Chen Ruohan】

## Intro to the project

The lightweight x86_64 kernel for learning how the operating system works, currently supports the full process from MBR boot to 64-bit kernel initialization.

## Separation of tasks

Tazieva Kamilla:

- README (editing)
- template version
- boot.asm, mbr.asm, kmain64.asm, linker.ld, boot.inc
- interruptions, keyboard handler (for input)
- virtual terminals
- ...


Chen Ruohan:

- README
- updated template version
- test version of interruptions
- 1st version of boot.asm and boot.inc, Makefile, kernel.c
- ...


## Project structure

```plaintext
LightNightOS/
├── src/
│ ├── boot/ # Bootloader code
│ ├── kernel/ # Kernel code
│ └── include/ # Common header files/macros definitions
├── build/ # Intermediate assembly products (generates automatically)
├── img/ # Disk image (generates automatically)
├── Makefile # Build script
└── README.md # Project description
```

## Dependencies on the environment

- Assembler: nasm
- Linker: ld
- Emulator: qemu-system-x86_64
- Other tools: dd, objcopy
