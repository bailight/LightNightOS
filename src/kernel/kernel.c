#include "../lib/stdint.h"
#include "memory.h"
#include "console.h"
#include "keyboard.h"
#include "interrupt.h"
#include "timer.h"

void kernel_init() {
    console_init();

    interrupt_init();

    keyboard_init();

    timer_init(100);

    print_str("Kernel initialized successfully.\n");
    print_str("This is LightNightOS.\n");

    printk("This test print: num=%d, hex=%x, str=%s\n", 123, 0xABC, "test");

    print_str_color("memory init --- \n", RED, BLACK);
    init_memory();

    print_str("\nKeyboard ready. Type here: ");
    print_str("\nTimer: dots should appear over time.\n");

    vt_render_active();
    sti();          // включаем прерывания

    while (1) {
        // всё делаем в хендлерах
    }
}

