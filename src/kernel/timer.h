#ifndef TIMER_H
#define TIMER_H

#include "../lib/stdint.h"

void timer_init(uint32_t frequency);
void timer_handler_c(void);
unsigned long long timer_ticks(void);

#endif

