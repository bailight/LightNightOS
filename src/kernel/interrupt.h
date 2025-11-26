#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "../lib/stdint.h"

extern void lidt(void* idtr);
extern void sti(void);

// Interrupt subsystem initialization
void interrupt_init(void);

// Register interrupt handlers
void idt_register_handler(int vector, void* handler);

#endif