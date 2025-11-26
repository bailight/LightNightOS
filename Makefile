# Путь к QEMU (архитектура x86_64 QEMU)

QEMU := qemu-system-x86_64

CFLAGS := -m64 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -O2 -Wall -Wextra

# Выходной каталог сборки

BUILD = ./build

# Каталог исходного кода

SRC = ./src

# Автоматически запускать цель "run" при выполнении "make" (сборка и загрузка системы)

all: run

# Инициализация каталога сборки
# Создать подкаталог для хранения артефактов сборки, обеспечивая его существование во время компиляции

build:
	mkdir -p $(BUILD)/boot $(BUILD)/kernel img/

# Скомпилировать все файлы .asm в каталоге src/boot в файлы .bin в каталоге build/boot

build/boot/%.bin: $(SRC)/boot/%.asm | build
	nasm -f bin $< -o $@ -I $(SRC)/include/

# Компилировать файл сборки запуска ядра в целевой файл ELF (для использования линкером)

build/kernel/kmain64.o: $(SRC)/kernel/kmain64.asm | build
	nasm -f elf64 $< -o $@ -I $(SRC)/include/

# Компилировать C файл ядра

build/kernel/kernel.o: $(SRC)/kernel/kernel.c | build
	gcc $(CFLAGS) -c $< -o $@ -I ../lib/

build/kernel/memory.o: $(SRC)/kernel/memory.c | build
	gcc $(CFLAGS) -c $< -o $@ -I $(SRC)/lib/

build/kernel/console.o: $(SRC)/kernel/console.c | build
	gcc $(CFLAGS) -c $< -o $@ -I $(SRC)/lib/

build/kernel/keyboard.o: $(SRC)/kernel/keyboard.c | build
	gcc $(CFLAGS) -c $< -o $@ -I $(SRC)/lib/

build/kernel/interrupt.o: $(SRC)/kernel/interrupt.c | build
	gcc $(CFLAGS) -c $< -o $@ -I $(SRC)/lib/

# Объединить целевой файл ядра со скриптом компоновки для создания исполняемого ядра в формате ELF

# build/kernel/kernel.elf: $(BUILD)/kernel/kmain64.o $(BUILD)/kernel/kernel.o
# 	ld -nostdlib -z max-page-size=0x1000 -T $(SRC)/kernel/linker.ld -o $@ $^

build/kernel/kernel.elf: $(BUILD)/kernel/kmain64.o $(BUILD)/kernel/kernel.o $(BUILD)/kernel/memory.o $(BUILD)/kernel/console.o $(BUILD)/kernel/interrupt.o $(BUILD)/kernel/keyboard.o
	ld -nostdlib -z max-page-size=0x1000 -T $(SRC)/kernel/linker.ld -o $@ $^

#Конвертировать ядро ​​формата ELF в чистый двоичный файл

build/kernel/kernel.bin: $(BUILD)/kernel/kernel.elf
	objcopy -O binary $< $@

# Создание образ диска, содержащий загрузчик и ядро ​​(имитация жёсткого диска)
# Шаг 1: Создать пустой файл образа размером 10 МБ (если = /dev/zero, он будет заполнен нулевыми данными)
# Шаг 2: Записать MBR (главную загрузочную запись) в сектор 0 (первый исполняемый файл, загруженный с жёсткого диска)
# Шаг 3: Записать boot.bin в следующий сектор (загрузчик второго уровня, загруженный MBR)
# Шаг 4: Записать kernel.bin в указанный сектор (ядро, загруженное загрузчиком)
# Параметры dd:
# bs=1M: Размер блока 1 МБ
# bs=512: Размер блока 512 байт (стандартный размер сектора диска)
# count=1: Записать только один блок (гарантирует, что MBR занимает только сектор 0)
# seek=1: Пропустить сектор 0 и начать запись с сектора 1 (чтобы избежать перезаписи MBR)

img/disk.img: $(BUILD)/boot/mbr.bin $(BUILD)/boot/boot.bin $(BUILD)/kernel/kernel.bin
	dd if=/dev/zero of=$@ bs=1M count=10 status=none
	dd if=$(BUILD)/boot/mbr.bin of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=$(BUILD)/kernel/kernel.bin of=$@ bs=512 seek=100 conv=notrunc status=none


# Загружение образ диска через эмулятор QEMU и запуск систему.

run: img/disk.img
	$(QEMU) -drive format=raw,file=img/disk.img -m 256M -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf build
