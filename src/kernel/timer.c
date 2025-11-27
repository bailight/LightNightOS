#include "timer.h"
#include "kernel.h"
#include "console.h"

#define PIT_CHANNEL0   0x40
#define PIT_COMMAND    0x43
#define PIT_FREQUENCY  1193182

#define PIC_MASTER_CMD 0x20

static volatile unsigned long long g_ticks = 0;

void timer_init(uint32_t frequency) {
    if (frequency == 0) {
        frequency = 100;
    }

    uint32_t divisor = PIT_FREQUENCY / frequency;

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

unsigned long long timer_ticks(void) {
    return g_ticks;
}

void timer_handler_c(void) {
    g_ticks++;

    if (g_ticks % 100 == 0) {
        print_str_color(".", VGA_LIGHT_GREY, VGA_BLACK);
    }

    outb(PIC_MASTER_CMD, 0x20);
}

