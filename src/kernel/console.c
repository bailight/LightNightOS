#include "console.h"
#include "../lib/stdint.h"

#define VGA_MEMORY 0xB8000
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

static uint8_t cursor_row = 0;
static uint8_t cursor_col = 0;
static uint8_t vga_color  = 0x07;

static uint16_t tty_buf[TTY_COUNT][VGA_WIDTH*VGA_HEIGHT];
static uint8_t  tty_row[TTY_COUNT], tty_col[TTY_COUNT], tty_color[TTY_COUNT];
static int      tty_cur = 0;

static inline uint16_t* TBUF(void) {
    return tty_buf[tty_cur];
}

static uint32_t vga_index(uint8_t row, uint8_t col) {
    return row * VGA_WIDTH + col;
}

void console_init(void) {
    clear_screen();
}

uint8_t get_cursor_row(void) {
    return cursor_row;
}

uint8_t get_cursor_col(void) {
    return cursor_col;
}

void set_cursor_row(uint8_t row) {
    if (row < VGA_HEIGHT) {
        cursor_row = row;
    }
}

void set_cursor_col(uint8_t col) {
    if (col < VGA_WIDTH) {
        cursor_col = col;
    }
}

void vt_render_active(void) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) {
        vga[i] = TBUF()[i];
    }
    update_cursor(cursor_row, cursor_col);
}

static void scroll_screen(void) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    uint16_t empty = (vga_color << 8) | ' ';

    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            uint32_t src_idx = vga_index(row, col);
            uint32_t dst_idx = vga_index(row-1, col);
            TBUF()[dst_idx] = TBUF()[src_idx];
            vga[dst_idx] = TBUF()[dst_idx];
        }
    }

    for (int col = 0; col < VGA_WIDTH; col++) {
        uint32_t idx = vga_index(VGA_HEIGHT-1, col);
        TBUF()[idx] = empty;
        vga[idx] = empty;
    }

    cursor_row = VGA_HEIGHT - 1;
    cursor_col = 0;
}

void clear_screen(void) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    uint16_t empty = (vga_color << 8) | ' ';

    for (int i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) {
        vga[i] = empty;
        TBUF()[i] = empty;
    }

    cursor_row = 0;
    cursor_col = 0;
    update_cursor(cursor_row, cursor_col);
}

// print char
void print_char(char c) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;

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
            uint16_t val = (vga_color << 8) | (uint8_t)c;
            TBUF()[idx] = val;
            vga[idx] = val;
            cursor_col++;
            break;
    }

    if (cursor_row >= VGA_HEIGHT) {
        scroll_screen();
    }
}

// Print string
void print_str(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

// Print 64-bit hexadecimal numbers
void print_hex64(uint64_t num) {
    print_str("0x");
    char hex_digits[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        uint8_t digit = (num >> (i * 4)) & 0x0F;
        print_char(hex_digits[digit]);
    }
}

// Print 64-bit unsigned integers
void print_uint64(uint64_t num) {
    if (num == 0) {
        print_char('0');
        return;
    }

    char buf[20];
    int idx = 0;
    
    while (num > 0) {
        buf[idx++] = '0' + (num % 10);
        num /= 10;
    }

    for (int i = idx - 1; i >= 0; i--) {
        print_char(buf[i]);
    }
}

// change terminal
void vt_switch(int tty_num) {
    if (tty_num < 0 || tty_num >= TTY_COUNT || tty_num == tty_cur) {
        return;
    }

    tty_row[tty_cur] = cursor_row;
    tty_col[tty_cur] = cursor_col;
    tty_color[tty_cur] = vga_color;

    tty_cur = tty_num;

    cursor_row = tty_row[tty_cur];
    cursor_col = tty_col[tty_cur];
    vga_color = tty_color[tty_cur];

    vt_render_active();
}

void update_cursor(uint8_t row, uint8_t col) {
    uint16_t cursor_pos = (uint16_t)row * VGA_WIDTH + col;

    outb(VGA_CTRL_PORT, 0x0E);
    outb(VGA_DATA_PORT, (cursor_pos >> 8) & 0xFF);

    outb(VGA_CTRL_PORT, 0x0F);
    outb(VGA_DATA_PORT, cursor_pos & 0xFF);  // cl = 低8位
}