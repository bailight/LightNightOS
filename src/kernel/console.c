#include <stdarg.h>
#include "console.h"
#include "lib.h"
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

static inline uint8_t vga_make_color(uint8_t fg, uint8_t bg) {
    return (bg << 4) | (fg & 0x0F);
}

// set vga color
void set_vga_color(uint8_t fg, uint8_t bg) {
    vga_color = vga_make_color(fg, bg);
    tty_color[tty_cur] = vga_color;
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
    print_char_color(c, VGA_LIGHT_GREY, VGA_BLACK);
}

void print_char_color(char c, uint8_t fg, uint8_t bg) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_MEMORY;
    uint8_t old_color = vga_color;
    set_vga_color(fg, bg);

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

    vga_color = old_color;
    tty_color[tty_cur] = old_color;
}

// Print string
void print_str(const char* str) {
    print_str_color(str, VGA_LIGHT_GREY, VGA_BLACK);
}

void print_str_color(const char* str, uint8_t fg, uint8_t bg) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char_color(str[i], fg, bg);
    }
}

// Print 64-bit unsigned integers
void print_uint64(uint64_t num) {
    print_uint64_color(num, VGA_LIGHT_GREY, VGA_BLACK);
}

void print_uint64_color(uint64_t num, uint8_t fg, uint8_t bg) {
    if (num == 0) {
        print_char_color('0', fg, bg);
        return;
    }

    char buf[20];
    int idx = 0;
    
    while (num > 0) {
        buf[idx++] = '0' + (num % 10);
        num /= 10;
    }

    for (int i = idx - 1; i >= 0; i--) {
        print_char_color(buf[i], fg, bg);
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
    outb(VGA_DATA_PORT, cursor_pos & 0xFF);
}

int skip_atoi(const char **s)
{
	int i=0; 

	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

static char fmt_buf[1024];

static char *number(char *str, uint64_t num, int base, int size, int precision, int type) {
    char c, sign, tmp[64];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i = 0;

    if (type & SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    if (type & LEFT) type &= ~ZEROPAD;
    if (base < 2 || base > 36) return NULL;

    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;

    if ((type & SIGN) && ((int64_t)num < 0)) {
        sign = '-';
        num = -(int64_t)num;
        size--;
    } else if (type & PLUS) {
        sign = '+';
        size--;
    } else if (type & SPACE) {
        sign = ' ';
        size--;
    }

    if (type & SPECIAL) {
        if (base == 16) size -= 2;
        else if (base == 8) size -= 1;
    }

    if (num == 0) {
        tmp[i++] = '0';
    } else {
        while (num != 0) {
            tmp[i++] = digits[num % base];
            num /= base;
        }
    }

    if (i > precision) precision = i;
    size -= precision;

    if (!(type & (ZEROPAD | LEFT))) {
        while (size-- > 0) *str++ = ' ';
    }

    if (sign) *str++ = sign;

    if (type & SPECIAL) {
        if (base == 8) {
            *str++ = '0';
        } else if (base == 16) {
            *str++ = '0';
            *str++ = (type & SMALL) ? 'x' : 'X';
        }
    }

    if (!(type & LEFT)) {
        while (size-- > 0) *str++ = c;
    }

    while (i < precision--) *str++ = '0';

    while (i-- > 0) *str++ = tmp[i];

    while (size-- > 0) *str++ = ' ';

    return str;
}

int vprintk(const char *fmt, va_list args) {
    char *str = fmt_buf;
    const char *s;
    int flags;
    int field_width;
    int precision;
    int qualifier; // h/l/L

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        flags = 0;
    repeat:
        fmt++;
        switch (*fmt) {
            case '-': flags |= LEFT; goto repeat;
            case '+': flags |= PLUS; goto repeat;
            case ' ': flags |= SPACE; goto repeat;
            case '#': flags |= SPECIAL; goto repeat;
            case '0': flags |= ZEROPAD; goto repeat;
        }

        field_width = -1;
        if (is_digit(*fmt)) {
            field_width = skip_atoi(&fmt);
        } else if (*fmt == '*') {
            fmt++;
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        precision = -1;
        if (*fmt == '.') {
            fmt++;
            if (is_digit(*fmt)) {
                precision = skip_atoi(&fmt);
            } else if (*fmt == '*') {
                fmt++;
                precision = va_arg(args, int);
            }
            if (precision < 0) precision = 0;
        }

        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            fmt++;
        }

        switch (*fmt) {
            case 'c': {
                if (!(flags & LEFT)) {
                    while (--field_width > 0) *str++ = ' ';
                }
                *str++ = (unsigned char)va_arg(args, int);
                while (--field_width > 0) *str++ = ' ';
                break;
            }

            case 's': {
                s = va_arg(args, const char*);
                if (!s) s = "(null)";
                int len = strlen(s);
                if (precision < 0) precision = len;
                else if (len > precision) len = precision;

                if (!(flags & LEFT)) {
                    while (len < field_width--) *str++ = ' ';
                }
                for (int i = 0; i < len; i++) *str++ = *s++;
                while (len < field_width--) *str++ = ' ';
                break;
            }

            case 'o': {
                uint64_t num = (qualifier == 'l') ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
                str = number(str, num, 8, field_width, precision, flags);
                break;
            }

            case 'p': {
                if (field_width == -1) {
                    field_width = 2 * sizeof(void*);
                    flags |= ZEROPAD;
                }
                str = number(str, (uint64_t)va_arg(args, void*), 16, field_width, precision, flags | SMALL);
                break;
            }

            case 'x': {
                flags |= SMALL;
                __attribute__((fallthrough)); 
            }
            case 'X': {
                uint64_t num = (qualifier == 'l') ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
                str = number(str, num, 16, field_width, precision, flags);
                break;
            }

            case 'd':
            case 'i': {
                flags |= SIGN;
                __attribute__((fallthrough)); 
            }
            case 'u': {
                uint64_t num = (qualifier == 'l') ? va_arg(args, uint64_t) : va_arg(args, uint32_t);
                str = number(str, num, 10, field_width, precision, flags);
                break;
            }

            case 'n': {
                if (qualifier == 'l') {
                    long *ip = va_arg(args, long*);
                    *ip = (str - fmt_buf);
                } else {
                    int *ip = va_arg(args, int*);
                    *ip = (str - fmt_buf);
                }
                break;
            }

            case '%': {
                *str++ = '%';
                break;
            }

            default: {
                *str++ = '%';
                if (*fmt) *str++ = *fmt;
                else fmt--;
                break;
            }
        }
    }

    *str = '\0';
    return str - fmt_buf;
}

int printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vprintk(fmt, args);
    va_end(args);

    print_str(fmt_buf);
    return len;
}

int printk_color(uint8_t fg, uint8_t bg, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vprintk(fmt, args);
    va_end(args);

    print_str_color(fmt_buf, fg, bg);
    return len;
}