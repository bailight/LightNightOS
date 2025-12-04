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

static void test_malloc_demo(void) {
    print_str("\n[TEST] malloc demo start\n");

    void *p1 = malloc(64);
    void *p2 = malloc(128);

    printk("[TEST] malloc(64) -> 0x%016lx", (unsigned long)p1);
    print_str("\n");

    printk("[TEST] malloc(128) -> 0x%016lx", (unsigned long)p2);
    print_str("\n");

    if (!p1 || !p2) {
        print_str("[TEST] malloc FAILED (null pointer)\n");
        return;
    }

    unsigned char *c1 = (unsigned char *)p1;
    for (int i = 0; i < 64; ++i) {
        c1[i] = (unsigned char)(i & 0xFF);
    }

    print_str("[TEST] wrote 64 bytes into p1, seems fine\n");
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
    
    // test_malloc_demo();
    
    scheduler_init();

    // process_create(worker_A, 0);
    // process_create(worker_B, 0);

    print_str("\nKeyboard ready. Type here: ");
    print_str("\nTimer: dots should appear over time.\n");

    vt_render_active();
    sti();          // interruptions on

    while (1) {
    	scheduler_tick();
        // everything in handlers
    }
}

