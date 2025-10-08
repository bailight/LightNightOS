QEMU := qemu-system-x86_64

all: run

build:
	mkdir -p build

build/mbr.bin: boot/mbr.asm | build
	nasm -f bin $< -o $@

build/boot.bin: boot/boot.asm | build
	nasm -f bin $< -o $@

build/kmain64.o: kernel/kmain64.asm | build
	nasm -f elf64 $< -o $@

build/kernel.elf: build/kmain64.o kernel/linker.ld
	ld -nostdlib -z max-page-size=0x1000 -T kernel/linker.ld -o $@ $<

build/kernel.bin: build/kernel.elf
	objcopy -O binary $< $@

build/disk.img: build/mbr.bin build/boot.bin build/kernel.bin
	dd if=/dev/zero of=$@ bs=1M count=10 status=none
	dd if=build/mbr.bin of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=build/boot.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=build/kernel.bin of=$@ bs=512 seek=100 conv=notrunc status=none

run: build/disk.img
	$(QEMU) -drive format=raw,file=build/disk.img -m 256M -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf build


