#include "scheduler.h"
#include "console.h"
#include "timer.h"
#include "memory.h"
#include "interrupts.h"

static process_t g_procs[MAX_PROCESSES];
static int       g_current = -1;
static uint64_t  g_next_pid = 1;
static unsigned long long g_last_switch_ticks = 0;
static int       g_scheduler_started = 0;

extern void context_switch_asm(process_context_t *old, process_context_t *new);

// Обёртка для процесса: вызывает entry, затем process_exit
static void process_wrapper(void) {
    if (g_current < 0 || g_current >= MAX_PROCESSES) {
        return;
    }
    
    process_t *proc = &g_procs[g_current];
    void (*entry)(void *) = (void (*)(void *))proc->context.rbx;
    void *arg = (void *)proc->context.r12;
    
    entry(arg);
    
    printk("[SCHED] Process %d finished\n", proc->pid);
    process_exit(0);
}

void idle_process(void *arg) {
    (void)arg;
    uint64_t pid = get_current_pid();
    printk("\n[IDLE] Idle process started (PID %d)\n", pid);
    
    while (1) {
        asm volatile("hlt");
    }
}

void scheduler_init(void) {
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        g_procs[i].pid   = 0;
        g_procs[i].state = PROC_UNUSED;
        g_procs[i].stack = NULL;
        g_procs[i].stack_size = 0;
        
        g_procs[i].context. rbx = 0;
        g_procs[i].context.rbp = 0;
        g_procs[i]. context.r12 = 0;
        g_procs[i].context.r13 = 0;
        g_procs[i].context.r14 = 0;
        g_procs[i].context. r15 = 0;
        g_procs[i]. context.rsp = 0;
        g_procs[i].context.rip = 0;
    }
    g_current = -1;
    g_next_pid = 1;
    g_last_switch_ticks = timer_ticks();
    g_scheduler_started = 0;
    
    printk("[SCHED] Scheduler initialized\n");
}

static int alloc_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i].state == PROC_UNUSED) {
            return i;
        }
    }
    return -1;
}

int process_create(void (*entry)(void *), void *arg) {
    int idx = alloc_slot();
    if (idx < 0) {
        printk("[SCHED] ERROR: No free process slots\n");
        return -1;
    }

    void *stack = malloc(STACK_SIZE);
    if (!stack) {
        printk("[SCHED] ERROR: Failed to allocate stack\n");
        return -1;
    }

    g_procs[idx].stack = stack;
    g_procs[idx].stack_size = STACK_SIZE;
    g_procs[idx].pid = g_next_pid++;
    g_procs[idx]. state = PROC_READY;

    uint64_t stack_top = ((uint64_t)stack + STACK_SIZE) & ~0xFULL;
    
    g_procs[idx]. context.rbx = (uint64_t)entry;
    g_procs[idx].context. r12 = (uint64_t)arg;
    g_procs[idx]. context.rbp = 0;
    g_procs[idx].context.r13 = 0;
    g_procs[idx].context.r14 = 0;
    g_procs[idx].context.r15 = 0;
    g_procs[idx].context.rsp = stack_top;
    g_procs[idx].context.rip = (uint64_t)process_wrapper;

    printk("[SCHED] Created process PID=%d in slot %d\n", g_procs[idx].pid, idx);

    return idx;
}

void process_yield(void) {
    if (! g_scheduler_started) {
        return;
    }

    if (g_current < 0) {
        return;
    }

    if (g_procs[g_current].state == PROC_RUNNING) {
        g_procs[g_current].state = PROC_READY;
    }
    
    int next = -1;
    int start = (g_current + 1) % MAX_PROCESSES;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        int idx = (start + i) % MAX_PROCESSES;
        if (g_procs[idx].state == PROC_READY) {
            next = idx;
            break;
        }
    }

    if (next < 0) {
        g_procs[g_current].state = PROC_RUNNING;
        return;
    }
    
    if (next == g_current) {
        g_procs[g_current]. state = PROC_RUNNING;
        return;
    }

    int old_current = g_current;
    g_current = next;
    g_procs[next].state = PROC_RUNNING;
    
    context_switch_asm(&g_procs[old_current].context, &g_procs[next].context);
}

void scheduler_tick(void) {
    // ОТКЛЮЧЕНО: не вызываем из обработчика прерываний
    // Это позволит клавиатуре работать нормально
    return;
}

uint64_t get_current_pid(void) {
    if (g_current >= 0 && g_current < MAX_PROCESSES && g_procs[g_current].state != PROC_UNUSED) {
        return g_procs[g_current].pid;
    }
    return 0;
}

void scheduler_start(void) {
    if (g_scheduler_started) {
        return;
    }
    
    printk("[SCHED] Starting scheduler.. .\n");
    
    int ready_count = 0;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i].state == PROC_READY) {
            ready_count++;
            printk("[SCHED] Found process PID %d in slot %d\n", g_procs[i].pid, i);
        }
    }
    
    if (ready_count == 0) {
        printk("[SCHED] No processes ready, creating idle\n");
        process_create(idle_process, 0);
    }
    
    int first = -1;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i].state == PROC_READY) {
            first = i;
            break;
        }
    }
    
    if (first < 0) {
        printk("[SCHED] ERROR: No processes to run!\n");
        return;
    }

    g_current = first;
    g_procs[first].state = PROC_RUNNING;
    g_scheduler_started = 1;
    
    printk("[SCHED] Starting with PID %d\n", g_procs[first].pid);
    printk("[SCHED] Jumping to first process.. .\n\n");
    
    context_switch_asm(NULL, &g_procs[first].context);
    
    printk("[SCHED] ERROR: Returned from scheduler_start!\n");
}

void process_exit(int status) {
    (void)status;
    
    if (g_current < 0) {
        return;
    }

    process_t *proc = &g_procs[g_current];
    uint64_t exiting_pid = proc->pid;
    
    printk("[SCHED] Process %d exiting\n", exiting_pid);
    
    if (proc->stack) {
        free(proc->stack);
    }
    
    proc->state = PROC_UNUSED;
    proc->pid = 0;
    proc->stack = NULL;
    proc->stack_size = 0;
    
    int next = -1;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i].state == PROC_READY || g_procs[i].state == PROC_RUNNING) {
            if (i != g_current) {
                next = i;
                break;
            }
        }
    }
    
    if (next < 0) {
        printk("[SCHED] No processes left, creating idle\n");
        next = process_create(idle_process, 0);
        if (next < 0) {
            printk("[SCHED] FATAL: Cannot create idle process!\n");
            g_scheduler_started = 0;
            g_current = -1;
            return;
        }
    }
    
    g_current = next;
    g_procs[next].state = PROC_RUNNING;
    
    printk("[SCHED] Switching to PID %d after exit\n", g_procs[next].pid);
    
    context_switch_asm(NULL, &g_procs[next].context);
}

int get_process_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i]. state != PROC_UNUSED) {
            count++;
        }
    }
    return count;
}

uint64_t get_process_pid(int index) {
    if (index >= 0 && index < MAX_PROCESSES && g_procs[index].state != PROC_UNUSED) {
        return g_procs[index].pid;
    }
    return 0;
}

proc_state_t get_process_state(int index) {
    if (index >= 0 && index < MAX_PROCESSES) {
        return g_procs[index].state;
    }
    return PROC_UNUSED;
}

void debug_print_processes(void) {
    printk("[SCHED] Process table (current=%d):\n", g_current);
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i]. state != PROC_UNUSED) {
            printk("  Slot %d: PID=%d, State=%d, Stack=0x%lx, RSP=0x%lx, RIP=0x%lx\n",
                   i, g_procs[i].pid, g_procs[i]. state,
                   (unsigned long)g_procs[i].stack,
                   (unsigned long)g_procs[i].context. rsp,
                   (unsigned long)g_procs[i].context.rip);
        }
    }
}
