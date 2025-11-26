#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdarg.h>
#include "../lib/stdint.h"
#include "kernel.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define TTY_COUNT  4

#define VGA_BLACK        0x00
#define VGA_BLUE         0x01
#define VGA_GREEN        0x02
#define VGA_CYAN         0x03
#define VGA_RED          0x04
#define VGA_MAGENTA      0x05
#define VGA_BROWN        0x06
#define VGA_LIGHT_GREY   0x07
#define VGA_DARK_GREY    0x08
#define VGA_LIGHT_BLUE   0x09
#define VGA_LIGHT_GREEN  0x0A
#define VGA_LIGHT_CYAN   0x0B
#define VGA_LIGHT_RED    0x0C
#define VGA_LIGHT_MAGENTA 0x0D
#define VGA_YELLOW       0x0E
#define VGA_WHITE        0x0F

#define WHITE   VGA_WHITE
#define BLACK   VGA_BLACK
#define RED     VGA_LIGHT_RED
#define ORANGE  VGA_BROWN
#define YELLOW  VGA_YELLOW
#define GREEN   VGA_LIGHT_GREEN
#define BLUE    VGA_LIGHT_BLUE
#define INDIGO  VGA_LIGHT_CYAN
#define PURPLE  VGA_LIGHT_MAGENTA

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

#define is_digit(c)	((c) >= '0' && (c) <= '9')

uint8_t get_cursor_row(void);
uint8_t get_cursor_col(void);
void set_cursor_row(uint8_t row);
void set_cursor_col(uint8_t col);

// uint8_t get_vga_color(void);
void set_vga_color(uint8_t fg, uint8_t bg);

void console_init(void);

void clear_screen(void);

// Print a single character
void print_char(char c);
void print_char_color(char c, uint8_t fg, uint8_t bg);

// Print string
void print_str(const char* str);
void print_str_color(const char* str, uint8_t fg, uint8_t bg);

// Print 64-bit unsigned integer
void print_uint64(uint64_t num);
void print_uint64_color(uint64_t num, uint8_t fg, uint8_t bg);

// Switch virtual terminal
void vt_switch(int tty_num);

void vt_render_active(void);

// Update cursor position
void update_cursor(uint8_t row, uint8_t col);

extern unsigned char font_ascii[256][16];

int skip_atoi(const char **s);
int vprintk(const char *fmt, va_list args);

// with variable
int printk(const char *fmt, ...);
int printk_color(uint8_t fg, uint8_t bg, const char *fmt, ...);

#endif