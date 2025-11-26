#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../lib/stdint.h"

// Keyboard interrupt handler
void keyboard_handler(void);

// Keyboard interrupt logic
void keyboard_handler_c(void);

// Initialize the keyboard
void keyboard_init(void);

#endif