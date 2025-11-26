#ifndef CONSOLE_H
#define CONSOLE_H

#include "../lib/stdint.h"
#include "kernel.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define TTY_COUNT  4

uint8_t get_cursor_row(void);
uint8_t get_cursor_col(void);
void set_cursor_row(uint8_t row);
void set_cursor_col(uint8_t col);

uint8_t get_vga_color(void);
void set_vga_color(uint8_t color);

void console_init(void);

void clear_screen(void);

// Print a single character
void print_char(char c);

// Print string
void print_str(const char* str);

// Print a 64-bit hexadecimal number
void print_hex64(uint64_t num);

// Print 64-bit unsigned integer
void print_uint64(uint64_t num);

// Switch virtual terminal
void vt_switch(int tty_num);

void vt_render_active(void);

// Update cursor position
void update_cursor(uint8_t row, uint8_t col);

#endif