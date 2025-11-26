#include "../lib/stdint.h"
#include "memory.h"
#include "console.h"
#include "keyboard.h"
#include "interrupt.h"  

void kernel_init() {
    console_init();

    interrupt_init();

    keyboard_init();

    print_str("Kernel initialized successfully.\n");
    print_str("This is LightNightOS.\n");
    print_str("\nKeyboard ready. Type here: ");
    
    vt_render_active();
    sti();

    for (;;) __asm__ __volatile__("hlt");
}