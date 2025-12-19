#include "../lib/stdint.h"
#include "memory.h"
#include "console.h"
#include "keyboard.h"
#include "interrupt.h"
#include "timer.h"
#include "interrupts.h"
#include "scheduler.h"
#include "fs/fs.h"
#include "fs/inode.h"
#include "fs/block.h"
#include "fs/dir.h"
#include "shell.h"

struct Global_Memory_Descriptor memory_management_struct = {0};

static int compute_factorial(int n) {
    if (n <= 1) return 1;
    return n * compute_factorial(n - 1);
}

static void process_A(void *arg) {
    (void)arg;
    uint64_t pid = get_current_pid();
    printk("[PID %d] Process A started\n", pid);
    
    for (int i = 0; i < 3; i++) {
        printk("[PID %d] A: iteration %d\n", pid, i);
        
        void *ptr = malloc(64);
        if (ptr) {
            printk("[PID %d] A: malloc OK\n", pid);
            free(ptr);
        }
        
        for (volatile int d = 0; d < 1000000; d++);
        
        process_yield();
    }
    
    printk("[PID %d] Process A finished\n", pid);
    process_exit(0);
}

static void process_B(void *arg) {
    (void)arg;
    uint64_t pid = get_current_pid();
    printk("[PID %d] Process B started\n", pid);
    
    for (int i = 0; i < 3; i++) {
        int fact = compute_factorial(i + 1);
        printk("[PID %d] B: factorial(%d) = %d\n", pid, i + 1, fact);
        
        for (volatile int d = 0; d < 1000000; d++);
        
        process_yield();
    }
    
    printk("[PID %d] Process B finished\n", pid);
    process_exit(0);
}

static void process_infinite(void *arg) {
    (void)arg;
    uint64_t pid = get_current_pid();
    printk("[PID %d] Background process started\n", pid);
    
    int counter = 0;
    while (1) {
        counter++;
        
        if (counter % 1000 == 0) {
            printk("[PID %d] Heartbeat: %d\n", pid, counter);
        }
        
        for (volatile int d = 0; d < 100000; d++);
        
        process_yield();
    }
}

static void test_malloc_demo(void) {
    print_str("\n[TEST] Simple malloc demo:\n");

    void *p1 = malloc(64);
    void *p2 = malloc(128);

    printk("[TEST] malloc(64) -> 0x%016lx\n", (unsigned long)p1);
    printk("[TEST] malloc(128) -> 0x%016lx\n", (unsigned long)p2);

    if (! p1 || !p2) {
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
}

static void init_filesystem() {
    fs_init();
    inode_cache_init();
    block_cache_init();
    
    printk("[TEST] Creating file.. .\n");
    int32_t file_inode = fs_create_file("test.txt");
    printk("[TEST] File created with inode: %d\n", file_inode);
    
    printk("[TEST] About to open file for writing...\n");
    uint32_t fd = fs_open("test.txt", FS_MODE_WRITE);
    printk("[TEST] fs_open returned fd: %d\n", fd);

    if (fd >= 0) {
        printk("[TEST] About to write to file...\n");
        int32_t bytes_written = fs_write(fd, "Hello FS!", 9);
        printk("[TEST] fs_write returned: %d\n", bytes_written);
        
        printk("[TEST] About to close file...\n");
        fs_close(fd);
        printk("[TEST] File closed\n");
    } else {
        printk("[TEST] ERROR: fs_open failed for writing\n");
    }

    // Create directory AFTER file operations
    printk("[TEST] Creating directory...\n");
    fs_create_dir("mydir");

    printk("[TEST] About to open file for reading...\n");
    uint32_t fd_read = fs_open("test.txt", FS_MODE_READ);
    printk("[TEST] fs_open for reading returned fd: %d\n", fd_read);

    if (fd_read >= 0) {
        printk("[TEST] Inside if block\n");
        char buffer[16];
        printk("[TEST] Buffer declared\n");
        buffer[0] = 0;
        printk("[TEST] Buffer initialized\n");
        int32_t bytes_read = 0;
        printk("[TEST] bytes_read initialized\n");
        printk("[TEST] About to call fs_read.. .\n");
        bytes_read = fs_read(fd_read, buffer, 9);
        printk("[TEST] fs_read returned: %d\n", bytes_read);
        
        printk("[TEST] File contents:  ");
        for (int i = 0; i < bytes_read; i++) {
            printk("%c", buffer[i]);
        }
        printk("\n");

        // Also display as string
        printk("[TEST] As string: %s\n", buffer);

        // Also print it in hex to debug
        printk("[TEST] File contents (hex): ");
        for (int i = 0; i < bytes_read; i++) {
            printk("%02x ", (uint8_t)buffer[i]);
        }
        printk("\n");

        // Print as string
        printk("[TEST] File contents (string): %s\n", buffer);
        
        printk("[TEST] About to close file...\n");
        fs_close(fd_read);
        printk("[TEST] File closed after reading\n");
    } else {
        printk("[TEST] ERROR: fs_open failed for reading\n");
    }

    printk("[TEST] All file operations complete!\n");
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

    // init_filesystem();
    
    // test_malloc_demo();
    
    // scheduler_init();
    
    
    // print_str("\nCreating test processes...\n");
    
    // int pid1 = process_create(process_A, 0);
    // printk("Created process A in slot %d\n", pid1);
    
    // int pid2 = process_create(process_B, 0);
    // printk("Created process B in slot %d\n", pid2);
    
    //// int pid3 = process_create(process_infinite, 0);
    //// printk("Created background process in slot %d\n", pid3);
    
    // debug_scheduler_info();
    
    // print_str("\n========================================\n");
    // print_str("Starting scheduler...\n");
    // print_str("Processes will run cooperatively (manual yield)\n");
    // print_str("You can still use keyboard and scroll!\n");
    // print_str("========================================\n\n");

    __asm__ volatile ("sti");

    shell_init();
    
    // scheduler_start();

    // Сюда мы не должны попасть
    while (1) {
        asm volatile("hlt");
    }
}
