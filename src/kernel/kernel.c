#include "../lib/stdint.h"
#include "memory.h"
#include "console.h"
#include "keyboard.h"
#include "interrupt.h"
#include "timer.h"
#include "interrupts.h"
#include "scheduler.h"

struct Global_Memory_Descriptor memory_management_struct = {0};

static void test_invalid_opcode(void) {
    __asm__ __volatile__ (".byte 0x0F, 0x0B"); // ud2
}

static void worker_A(void *arg) {
    (void)arg;
    print_str_color("A", VGA_LIGHT_GREEN, VGA_BLACK);
}

static void worker_B(void *arg) {
    (void)arg;
    print_str_color("B", VGA_LIGHT_CYAN, VGA_BLACK);
}

void kernel_init() {
    idt_init();
    
    // test_invalid_opcode(); // crash! (okay, i really meant to make it)
    
    console_init();

    interrupt_init();

    keyboard_init();

    timer_init(100);

    print_str("Kernel initialized successfully.\n");
    print_str("This is LightNightOS.\n");

    printk("This test print: num=%d, hex=%x, str=%s\n", 123, 0xABC, "test");
    
    init_memory();
    
    scheduler_init();

    process_create(worker_A, 0);
    process_create(worker_B, 0);

    print_str("\nKeyboard ready. Type here: ");
    print_str("\nTimer: dots should appear over time.\n");

    vt_render_active();
    sti();          // interruptions on

    while (1) {
    	scheduler_tick();
        // everything in handlers
    }
}

