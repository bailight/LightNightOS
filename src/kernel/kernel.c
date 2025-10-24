#include "../lib/stdint.h"

extern void update_cursor(uint8_t row, uint8_t col);
extern void lidt(void* idtr);
extern void sti();
extern void timer_handler();
extern void keyboard_handler(); 

#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD   0xA0
#define PIC_SLAVE_DATA  0xA1
#define KEYBOARD_PORT 0x60

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

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile ("outb %0, %1" : : "a"(data), "d"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile ("inb %1, %0" : "=a"(data) : "d"(port));
    return data;
}

static char scancode_to_char(uint8_t scancode) {
    static char map[] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };
    if (scancode < sizeof(map)) return map[scancode];
    return 0;
}

void keyboard_handler_c() {
    uint8_t scancode = inb(KEYBOARD_PORT);

    if (scancode & 0x80) {
        outb(PIC_MASTER_CMD, 0x20);  // EOI
        return;
    }

    char c = scancode_to_char(scancode);
    if (c) {
        print_char(c);
    } else if (scancode == 0x0E) {  
        if (cursor_col > 0) {
            cursor_col--;
            print_char(' ');
            cursor_col--;
        }
    }

    update_cursor(cursor_row, cursor_col);
    outb(PIC_MASTER_CMD, 0x20);
}

typedef struct {
    uint16_t offset_low;
    uint16_t segment;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;


idt_entry_t idt[256] = {0};

void idt_set_gate(int vector, void* handler) {
    uint64_t addr = (uint64_t)handler;
    idt[vector].offset_low  = addr & 0xFFFF;
    idt[vector].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].segment     = 0x08;
    idt[vector].ist         = 1;
    idt[vector].flags       = 0x8F;
}

void idt_init() {
    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) idtr = {
        .limit = sizeof(idt) - 1,
        .base  = (uint64_t)idt
    };

    idt_set_gate(33, keyboard_handler);
    lidt(&idtr);
}

void pic_init() {
    outb(PIC_MASTER_CMD, 0x11);
    outb(PIC_SLAVE_CMD, 0x11);
    outb(PIC_MASTER_DATA, 0x20);
    outb(PIC_SLAVE_DATA, 0x28);
    outb(PIC_MASTER_DATA, 0x04);
    outb(PIC_SLAVE_DATA, 0x02);
    outb(PIC_MASTER_DATA, 0x01);
    outb(PIC_SLAVE_DATA, 0x01);

    outb(PIC_MASTER_DATA, 0xFE);
    outb(PIC_SLAVE_DATA, 0xFF);
}


void kernel_init(void) {
    idt_init();
    pic_init();
    
    clear_screen();

    print_str("Hello, QEMU!\n");
    print_str("This is a test for VGA text mode.\n");
    print_str("Kernel initialized successfully.\n");
    print_str("This is LightNightOS.\n");

    // sti();
    
    for (;;) __asm__ __volatile__("hlt");
}
