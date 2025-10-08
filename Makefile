QEMU := qemu-system-x86_64

all: run

build:
	mkdir -p build/boot build/kernel

build/boot/mbr.bin: src/boot/mbr.asm | build
	nasm -f bin $< -o $@ -I src/include/

build/boot/boot.bin: src/boot/boot.asm | build
	nasm -f bin $< -o $@ -I src/include/

build/kernel/kmain64.o: src/kernel/kmain64.asm | build
	nasm -f elf64 $< -o $@ -I src/include/

build/kernel/kernel.elf: build/kernel/kmain64.o src/kernel/linker.ld
# 	ld -nostdlib -m elf_x86_64 -T src/kernel/linker.ld -o build/kernel/kernel.elf build/kernel/kmain64.o
	ld -nostdlib -z max-page-size=0x1000 -T src/kernel/linker.ld -o $@ $<

build/kernel/kernel.bin: build/kernel/kernel.elf
	objcopy -O binary $< $@

img/disk.img: build/boot/mbr.bin build/boot/boot.bin build/kernel/kernel.bin
	dd if=/dev/zero of=$@ bs=1M count=10 status=none
	dd if=build/boot/mbr.bin of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=build/boot/boot.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=build/kernel/kernel.bin of=$@ bs=512 seek=100 conv=notrunc status=none

run: img/disk.img
	$(QEMU) -drive format=raw,file=img/disk.img -m 256M -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf build
