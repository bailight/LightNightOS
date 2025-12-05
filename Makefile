QEMU := qemu-system-x86_64
CC := gcc
LD := ld
NASM := nasm

CFLAGS := -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
          -mcmodel=kernel -O2 -Wall -Wextra -I./src
LDFLAGS := -nostdlib -z max-page-size=0x1000
NASMFLAGS := -f elf64

BUILD := ./build
SRC := ./src

BOOT_DIR := $(SRC)/boot
KERNEL_DIR := $(SRC)/kernel

all: run

build:
	mkdir -p $(BUILD)/boot
	mkdir -p $(BUILD)/kernel
	mkdir -p img

$(BUILD)/boot/mbr.bin: $(BOOT_DIR)/mbr.asm | build
	$(NASM) -f bin $< -o $@

$(BUILD)/boot/boot.bin: $(BOOT_DIR)/boot.asm | build
	$(NASM) -f bin $< -o $@

# Ассемблерные файлы ядра
KERNEL_ASM_SRCS := $(KERNEL_DIR)/kmain64.asm \
                   $(KERNEL_DIR)/isr.asm \
                   $(KERNEL_DIR)/context_switch.asm

KERNEL_ASM_OBJS := $(KERNEL_ASM_SRCS:$(KERNEL_DIR)/%.asm=$(BUILD)/kernel/%.o)

$(BUILD)/kernel/%.o: $(KERNEL_DIR)/%.asm | build
	$(NASM) $(NASMFLAGS) $< -o $@

# C файлы ядра
KERNEL_C_SRCS := $(KERNEL_DIR)/kernel.c \
                 $(KERNEL_DIR)/memory.c \
                 $(KERNEL_DIR)/console.c \
                 $(KERNEL_DIR)/keyboard.c \
                 $(KERNEL_DIR)/interrupt.c \
                 $(KERNEL_DIR)/interrupts.c \
                 $(KERNEL_DIR)/timer.c \
                 $(KERNEL_DIR)/scheduler.c

KERNEL_C_OBJS := $(KERNEL_C_SRCS:$(KERNEL_DIR)/%.c=$(BUILD)/kernel/%.o)

$(BUILD)/kernel/%.o: $(KERNEL_DIR)/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel/kernel.elf: $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS) $(KERNEL_DIR)/linker.ld | build
	$(LD) $(LDFLAGS) -T $(KERNEL_DIR)/linker.ld -o $@ $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

$(BUILD)/kernel/kernel.bin: $(BUILD)/kernel/kernel.elf | build
	objcopy -O binary $< $@

img/disk.img: $(BUILD)/boot/mbr.bin $(BUILD)/boot/boot.bin $(BUILD)/kernel/kernel.bin | build
	dd if=/dev/zero of=$@ bs=1M count=10 status=none
	dd if=$(BUILD)/boot/mbr.bin of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=$(BUILD)/kernel/kernel.bin of=$@ bs=512 seek=100 conv=notrunc status=none

run: img/disk.img
	$(QEMU) -drive format=raw,file=img/disk.img -m 2G -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD) img/disk.img

.PHONY: all build run clean
