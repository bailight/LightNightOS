#ifndef TIMER_H
#define TIMER_H

#include "../lib/stdint.h"

struct regs_t;  // forward-decl, чтобы не тянуть весь interrupts.h сюда

void timer_init(uint32_t frequency);
void timer_handler_c(struct regs_t *regs);
unsigned long long timer_ticks(void);

#endif

