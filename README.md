# LightNightOS
OS project 【Tazieva Kamilla & Chen Ruohan】

## Intro to the project

The lightweight x86_64 kernel for learning how the operating system works, currently supports the full process from MBR boot to 64-bit kernel initialization.

## Separation of tasks

Tazieva Kamilla:

- template version
- boot.asm, mbr.asm, kmain64.asm, linker.ld, boot.inc, updated kernel.c
- interruptions, keyboard handler (for input)
- Multithreading
- Processes (multiple process slices running simultaneously)
- File system: creating files and folders: has superblock wih block size, total blocks, inodes, bitmaps; separate inode table with metadata; 12 direct blocks + 1 indirect blocks; block bitmap and inode bitmap; has dir_entry_t structure; in-memory; file descriptors


Chen Ruohan:

- updated template version
- test version of interruptions
- 1st version of boot.asm and boot.inc, Makefile, kernel.c
- Memory management: init_pages
- Memory management: malloc and free
- Minor features: screen scroll
- Simple shell in the kernel (currently no user space, includes minor features: help, echo, clean, up and down cursor to view command history, left and right cursor to move input)


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


FS Structure: 
┌──────────────────────────────────────────┐
│         Superblock (metadata)            │
│  - Block size: 4096 bytes                │
│  - Total blocks: 10240                   │
│  - Total inodes: 2048                    │
└──────────────────────────────────────────┘
┌──────────────────────────────────────────┐
│    Block Bitmap (tracks free blocks)     │
└──────────────────────────────────────────┘
┌──────────────────────────────────────────┐
│    Inode Bitmap (tracks free inodes)     │
└──────────────────────────────────────────┘
┌──────────────────────────────────────────┐
│         Inode Table (file metadata)      │
│  - Inode 0: Root directory               │
│  - Inode 1: test.txt                     │
│  - Inode 2: mydir                        │
└──────────────────────────────────────────┘
┌──────────────────────────────────────────┐
│         Data Blocks (file contents)      │
│  Currently:  in-memory (malloc'd)        │
└──────────────────────────────────────────┘


## Dependencies on the environment

- Assembler: nasm
- Linker: ld
- Emulator: qemu-system-x86_64
- Other tools: dd, objcopy
