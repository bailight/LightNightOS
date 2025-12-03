#pragma once

#include "../lib/stdint.h"

void scheduler_init(void);

int  process_create(void (*entry)(void *), void *arg);

void scheduler_tick(void);

void process_yield(void);

