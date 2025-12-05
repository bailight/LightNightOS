#include "../lib/stdint.h"
#include "memory.h"
#include "console.h"
#include "keyboard.h"
#include "interrupt.h"
#include "timer.h"
#include "interrupts.h"
#include "scheduler.h"

struct Global_Memory_Descriptor memory_management_struct = {0};

static int compute_factorial(int n) {
    if (n <= 1) return 1;
    return n * compute_factorial(n - 1);
}

static void process_A(void *arg) {
    (void)arg;
    uint64_t pid = get_current_pid();
    printk("\n[PID %d] Process A STARTED\n", pid);
    
    for (int i = 0; i < 5; i++) {
        printk("[PID %d] A: iteration %d\n", pid, i);
        
        void *ptr = malloc(64);
        if (ptr) {
            printk("[PID %d] A: malloc OK at 0x%lx\n", pid, (unsigned long)ptr);
            free(ptr);
        }
        
        for (volatile int d = 0; d < 100000; d++);
        
        printk("[PID %d] A: calling yield\n", pid);
        process_yield();
    }
    
    printk("[PID %d] Process A FINISHED\n", pid);
    process_exit(0);
}

static void process_B(void *arg) {
    (void)arg;
    uint64_t pid = get_current_pid();
    printk("\n[PID %d] Process B STARTED\n", pid);
    
    for (int i = 0; i < 5; i++) {
        printk("[PID %d] B: iteration %d\n", pid, i);
        
        int fact = compute_factorial(i + 1);
        printk("[PID %d] B: factorial(%d) = %d\n", pid, i + 1, fact);
        
        for (volatile int d = 0; d < 100000; d++);
        
        printk("[PID %d] B: calling yield\n", pid);
        process_yield();
    }
    
    printk("[PID %d] Process B FINISHED\n", pid);
    process_exit(0);
}

static void test_malloc_demo(void) {
    print_str("\n[TEST] Simple malloc demo:\n");

    void *p1 = malloc(64);
    void *p2 = malloc(128);

    printk("[TEST] malloc(64) -> 0x%016lx\n", (unsigned long)p1);
    printk("[TEST] malloc(128) -> 0x%016lx\n", (unsigned long)p2);

    if (!p1 || !p2) {
        print_str("[TEST] malloc FAILED\n");
        return;
    }

    print_str("[TEST] malloc OK\n");
    
    free(p1);
    free(p2);
}

static void debug_scheduler_info(void) {
    print_str("\n[SCHED] Debug information:\n");
    
    int count = get_process_count();
    printk("[SCHED] Active processes: %d\n", count);
    
    debug_print_processes();
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        proc_state_t state = get_process_state(i);
        if (state != PROC_UNUSED) {
            uint64_t pid = get_process_pid(i);
            printk("[SCHED] Slot %d: PID=%d, State=%d\n", i, pid, state);
        }
    }
}

void kernel_init() {
    idt_init();
    console_init();
    interrupt_init();
    keyboard_init();
    timer_init(100);
    
    print_str("\n========================================\n");
    print_str("LightNightOS Kernel Initialized\n");
    print_str("========================================\n\n");
    
    init_memory();
    test_malloc_demo();
    
    scheduler_init();
    
    print_str("\nCreating idle process...\n");
    int idle_slot = process_create(idle_process, 0);
    printk("Created idle process in slot %d\n", idle_slot);
    
    print_str("\nCreating test processes...\n");
    
    int pid1 = process_create(process_A, 0);
    printk("Created process A in slot %d\n", pid1);
    
    int pid2 = process_create(process_B, 0);
    printk("Created process B in slot %d\n", pid2);
    
    debug_scheduler_info();
    
    print_str("\nStarting scheduler...\n");
    print_str("Press SPACE to manually trigger yield\n");
    print_str("========================================\n");

    sti();
    
    scheduler_start();
    
    while (1) {
        asm volatile("hlt");
    }
}
