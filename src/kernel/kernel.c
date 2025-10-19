typedef unsigned short uint16_t;

void kernel_init() {
    uint16_t *vga = (uint16_t*)0xB8000;
    vga[10] = 0x074F;  // 第 1 行第 0 列打印 'O'（白字黑底）
    vga[11] = 0x074B;  // 第 1 行第 1 列打印 'K'

    while (1) {
        __asm__ __volatile__("hlt");
    }
}