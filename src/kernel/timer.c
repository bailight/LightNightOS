#include "timer.h"
#include "kernel.h"
#include "console.h"
#include "scheduler.h"
#include "interrupts.h"

#define PIT_CHANNEL0   0x40
#define PIT_COMMAND    0x43
#define PIT_FREQUENCY  1193182

static volatile unsigned long long g_ticks = 0;

void timer_init(uint32_t frequency) {
    if (frequency == 0) {
        frequency = 100;
    }

    uint32_t divisor = PIT_FREQUENCY / frequency;

    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    g_ticks = 0;
}

unsigned long long timer_ticks(void) {
    return g_ticks;
}

void timer_handler_c(struct regs_t *regs) {
    (void)regs;

    g_ticks++;

    scheduler_tick();

    outb(0x20, 0x20);
}
