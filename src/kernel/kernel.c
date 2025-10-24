#include "../lib/stdint.h"

extern void update_cursor(uint8_t row, uint8_t col);
extern void lidt(void* idtr);
extern void sti(void);
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

#define PS2_DATA    0x60
#define PS2_STATUS  0x64

static uint8_t cursor_row = 0;
static uint8_t cursor_col = 0;
static uint8_t vga_color  = 0x07;

/* ===== Virtual Terminals (F1..F4) ===== */
#define TTY_COUNT 4
static uint16_t tty_buf[TTY_COUNT][VGA_WIDTH*VGA_HEIGHT];
static uint8_t  tty_row[TTY_COUNT], tty_col[TTY_COUNT], tty_color[TTY_COUNT];
static int      tty_cur = 0;


static inline uint16_t* TBUF(void){ return tty_buf[tty_cur]; }

static void vt_render_active(void){
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    for (int i=0;i<VGA_WIDTH*VGA_HEIGHT;i++) vga[i] = TBUF()[i];
    update_cursor(cursor_row, cursor_col);
}

static void vt_switch(int n){
    if (n < 0 || n >= TTY_COUNT || n == tty_cur) return;
    /* saving old TTY state */
    tty_row[tty_cur] = cursor_row;
    tty_col[tty_cur] = cursor_col;

    /* switch */
    tty_cur = n;

    /* restore new TTY state */
    cursor_row = tty_row[tty_cur];
    cursor_col = tty_col[tty_cur];
    vga_color  = tty_color[tty_cur];

    vt_render_active();
}

static uint32_t vga_index(uint8_t row, uint8_t col) {
    return row * VGA_WIDTH + col;
}

void clear_screen() {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    uint16_t empty = (vga_color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i]     = empty;
        TBUF()[i]  = empty;
    }
    cursor_row = 0;
    cursor_col = 0;
    update_cursor(cursor_row, cursor_col);
}

static void scroll_screen() {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            int src = row * VGA_WIDTH + col;
            int dst = (row - 1) * VGA_WIDTH + col;
            TBUF()[dst] = TBUF()[src];
            vga[dst]    = TBUF()[dst];
        }
    }
    for (int col = 0; col < VGA_WIDTH; col++) {
        int idx = (VGA_HEIGHT - 1) * VGA_WIDTH + col;
        TBUF()[idx] = (vga_color << 8) | ' ';
        vga[idx]    = TBUF()[idx];
    }
    cursor_row = VGA_HEIGHT - 1;
    cursor_col = 0;
}

void print_char(char c) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    switch (c) {
        case '\n': cursor_row++; cursor_col = 0; break;
        case '\r': cursor_col = 0; break;
        case '\t': cursor_col = (cursor_col + 4) & ~3; break;
        default:
            if (cursor_col >= VGA_WIDTH) { cursor_col = 0; cursor_row++; }
            {
                int idx = vga_index(cursor_row, cursor_col);
                uint16_t val = (vga_color << 8) | (uint8_t)c;
                TBUF()[idx] = val;   // to TTY buffer
                vga[idx]    = val;   // to the screen
                cursor_col++;
            }
            break;
    }
    if (cursor_row >= VGA_HEIGHT) scroll_screen();
}

void print_str(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(data), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

/* --- clearing buffer 8042 before turning on IRQ --- */
static void kbd_flush(void){
    for (int i = 0; i < 256; ++i) {
        uint8_t st = inb(PS2_STATUS);
        if (st & 0x01) (void)inb(PS2_DATA);
        else break;
    }
}

static char scancode_to_char(uint8_t scancode) {
    static char map[] = {
        0,  27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
        'a','s','d','f','g','h','j','k','l',';','\'','`',   0,'\\',
        'z','x','c','v','b','n','m',',','.','/',           0,   0,   0,' '
    };
    if (scancode < sizeof(map)) return map[scancode];
    return 0;
}

/* --- keyboard handler: read ALL pending bytes --- */
void keyboard_handler_c() {
    while (inb(PS2_STATUS) & 0x01) {
        uint8_t scancode = inb(PS2_DATA);

        if (scancode & 0x80) {
            continue; // ignoting break-codes
        }
        
        if (!(scancode & 0x80) && scancode >= 0x3B && scancode <= 0x3E) {
            vt_switch(scancode - 0x3B);  // 0..3
            continue;
        }

        if (scancode == 0x0E) { // Backspace
            if (cursor_col > 0) {
                cursor_col--;
                print_char(' ');
                cursor_col--;
            }
        } else {
            char c = scancode_to_char(scancode);
            if (c) print_char(c);
        }
    }

    update_cursor(cursor_row, cursor_col);
    outb(PIC_MASTER_CMD, 0x20);  // EOI mastery
}

/* ---------------- IDT ---------------- */
typedef struct {
    uint16_t offset_low;
    uint16_t segment;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

static idt_entry_t idt[256] = {0};

static void idt_set_gate(int vector, void* handler) {
    uint64_t addr = (uint64_t)handler;
    idt[vector].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[vector].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vector].segment     = 0x18;   // 64-bit code selector from GDT
    idt[vector].ist         = 0;      // not using IST
    idt[vector].flags       = 0x8E;   // P=1,DPL=0, Type=0xE (interrupt gate)
    idt[vector].reserved    = 0;
}

static void idt_init(void) {
    // array already zeroed, then only IRQ1 (33 = 0x21)
    idt_set_gate(33, keyboard_handler);

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) idtr = {
        .limit = sizeof(idt) - 1,
        .base  = (uint64_t)idt
    };
    lidt(&idtr);
}

/* ---------------- PIC ---------------- */
static void pic_init(void) {
    // shutting all IRQ during remap
    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA,  0xFF);

    outb(PIC_MASTER_CMD,  0x11);
    outb(PIC_SLAVE_CMD,   0x11);
    outb(PIC_MASTER_DATA, 0x20);   // master offset 0x20
    outb(PIC_SLAVE_DATA,  0x28);   // slave  offset 0x28
    outb(PIC_MASTER_DATA, 0x04);   // slave на IRQ2
    outb(PIC_SLAVE_DATA,  0x02);
    outb(PIC_MASTER_DATA, 0x01);   // 8086 mode
    outb(PIC_SLAVE_DATA,  0x01);

    outb(PIC_MASTER_DATA, 0xFD);   // 1111 1101b -> IRQ1 allowed
    outb(PIC_SLAVE_DATA,  0xFF);   // everything is closed
}

/* ---------------- entry ---------------- */
void kernel_init(void) {
    /* init VT state */
    for (int t=0; t<TTY_COUNT; ++t) {
        tty_color[t] = 0x07;
        tty_row[t] = tty_col[t] = 0;
        uint16_t empty = (tty_color[t] << 8) | ' ';
        for (int i=0; i<VGA_WIDTH*VGA_HEIGHT; ++i)
            tty_buf[t][i] = empty;
    }
    
    tty_cur = 0;
    idt_init();
    pic_init();

    clear_screen();
    print_str("Hello, QEMU!\n");
    print_str("This is a test for VGA text mode.\n");
    print_str("Kernel initialized successfully.\n");
    print_str("This is LightNightOS.\n");
    print_str("\nKeyboard ready. Type here: ");
    
    vt_render_active();

    // clear the controller buffer before enabling interrupts
    kbd_flush();

    sti();

    for (;;) __asm__ __volatile__("hlt");
}

