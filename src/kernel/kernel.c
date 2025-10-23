#include "../lib/stdint.h"

extern void update_cursor(uint8_t row, uint8_t col);

#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

static uint8_t cursor_row = 0;
static uint8_t cursor_col = 0;

static uint8_t vga_color = 0x07;

static uint32_t vga_index(uint8_t row, uint8_t col) {
    return row * VGA_WIDTH + col;
}


void clear_screen() {
    uint16_t* vga_buf = (uint16_t*)VGA_MEMORY;
    uint16_t empty_char = (vga_color << 8) | ' ';

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buf[i] = empty_char;
    }
    cursor_row = 0;
    cursor_col = 0;

    update_cursor(cursor_row, cursor_col);
}

// 滚动屏幕（当行满时）
static void scroll_screen() {
    
    uint16_t* vga_buf = (uint16_t*)VGA_MEMORY;
    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            uint32_t src_idx = vga_index(row, col);
            uint32_t dst_idx = vga_index(row - 1, col);
            vga_buf[dst_idx] = vga_buf[src_idx];
        }
    }

    for (int col = 0; col < VGA_WIDTH; col++) {
        uint32_t idx = vga_index(VGA_HEIGHT - 1, col);
        vga_buf[idx] = (vga_color << 8) | ' ';
    }

    cursor_row = VGA_HEIGHT - 1;
    cursor_col = 0;

}

void print_char(char c) {
    uint16_t* vga_buf = (uint16_t*)VGA_MEMORY;

    switch (c) {
        case '\n':
            cursor_row++;
            cursor_col = 0;
            break;
        case '\r':
            cursor_col = 0;
            break;
        case '\t':
            cursor_col = (cursor_col + 4) & ~3;
            break;
        default:
            if (cursor_col >= VGA_WIDTH) {
                cursor_col = 0;
                cursor_row++;
            }
            uint32_t idx = vga_index(cursor_row, cursor_col);
            vga_buf[idx] = (vga_color << 8) | (uint8_t)c;
            cursor_col++;
            break;
    }

    if (cursor_row >= VGA_HEIGHT) {
        scroll_screen();
    }

}

void print_str(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

void kernel_init() {
    clear_screen();

    print_str("Hello, QEMU!\n");
    print_str("This is a test for VGA text mode.\n");
    print_str("Kernel initialized successfully.\n");
    print_str("This is LightNightOS.");

    cursor_row = 4;
    cursor_col = 0;

    update_cursor(cursor_row, cursor_col);

    while (1) {
        __asm__ __volatile__("hlt"); 
    }
}