#include "scheduler.h"
#include "console.h"
#include "timer.h"
#include "memory.h"
#include "interrupts.h"

static process_t g_procs[MAX_PROCESSES];
static int       g_current = -1;
static uint64_t  g_next_pid = 1;
static unsigned long long g_last_switch_ticks = 0;
static int      g_scheduler_started = 0;

extern void context_switch_asm(process_context_t **old, process_context_t *new);

void idle_process(void *arg) {
    (void)arg;
    uint64_t pid = get_current_pid();
    printk("\n[IDLE] Idle process started (PID %d)\n", pid);
    while (1) {
        asm volatile("pause");
        if (g_scheduler_started) {
            process_yield();
        }
    }
}

void context_switch(process_context_t **old, process_context_t *new) {
    printk("[SCHED] Switching context: old_ptr=0x%lx, old=0x%lx, new=0x%lx\n", 
           (unsigned long)old, 
           (unsigned long)(old ? *old : NULL),
           (unsigned long)new);
    context_switch_asm(old, new);
}

void scheduler_init(void) {
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        g_procs[i].pid   = 0;
        g_procs[i].state = PROC_UNUSED;
        g_procs[i].context = NULL;
        g_procs[i].stack = NULL;
        g_procs[i].stack_size = 0;
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

static void init_process_context(process_t *proc, void (*entry)(void *), void *arg) {
    uint64_t stack_base = (uint64_t)proc->stack;
    uint64_t stack_size = proc->stack_size;
    
    proc->context = (process_context_t*)(stack_base + stack_size - sizeof(process_context_t));
    
    uint64_t stack_top = stack_base + stack_size;
    stack_top = stack_top & ~0xF;
    
    for (size_t i = 0; i < sizeof(process_context_t); i++) {
        ((uint8_t*)proc->context)[i] = 0;
    }
    
    proc->context->rip = (uint64_t)entry;
    proc->context->cs = 0x08;
    proc->context->rflags = 0x202;
    proc->context->rsp = stack_top;
    proc->context->ss = 0x10;
}

int process_create(void (*entry)(void *), void *arg) {
    int idx = alloc_slot();
    if (idx < 0) {
        return -1;
    }

    void *stack = malloc(STACK_SIZE);
    if (!stack) {
        return -1;
    }

    g_procs[idx].pid   = g_next_pid++;
    g_procs[idx].state = PROC_READY;
    g_procs[idx].stack = stack;
    g_procs[idx].stack_size = STACK_SIZE;

    init_process_context(&g_procs[idx], entry, arg);

    return idx;
}

void process_yield(void) {
    if (!g_scheduler_started) {
        return;
    }

    if (g_current < 0) {
        return;
    }

    uint64_t current_pid = g_procs[g_current].pid;
    
    if (g_procs[g_current].state == PROC_RUNNING) {
        g_procs[g_current].state = PROC_READY;
    }
    
    int next = -1;
    int start = (g_current + 1) % MAX_PROCESSES;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        int idx = (start + i) % MAX_PROCESSES;
        if (g_procs[idx].state == PROC_READY && idx != g_current) {
            next = idx;
            break;
        }
    }

    if (next < 0) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (g_procs[i].state != PROC_UNUSED && i != g_current) {
                next = i;
                g_procs[next].state = PROC_READY;
                break;
            }
        }
    }

    if (next < 0 || next == g_current) {
        g_procs[g_current].state = PROC_RUNNING;
        return;
    }

    int old_current = g_current;
    g_procs[next].state = PROC_RUNNING;
    g_current = next;
    
    context_switch(&g_procs[old_current].context, g_procs[next].context);
}

void scheduler_tick(void) {
    if (!g_scheduler_started) {
        return;
    }

    unsigned long long now = timer_ticks();
    if (now - g_last_switch_ticks < TIME_SLICE_TICKS) {
        return;
    }
    g_last_switch_ticks = now;

    process_yield();
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
    
    printk("[SCHED] Starting scheduler...\n");
    
    int ready_count = 0;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i].state == PROC_READY) {
            ready_count++;
            printk("[SCHED] Found process PID %d in slot %d (state=%d)\n", 
                   g_procs[i].pid, i, g_procs[i].state);
        }
    }
    
    if (ready_count == 0) {
        process_create(idle_process, 0);
    }
    
    int first = -1;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i].state == PROC_READY) {
            first = i;
            break;
        }
    }
    
    if (first >= 0) {
        g_current = first;
        g_procs[first].state = PROC_RUNNING;
        g_scheduler_started = 1;
        
        printk("[SCHED] Starting with PID %d in slot %d\n", g_procs[first].pid, first);
        printk("[SCHED] Switching to first process...\n");
        
        process_context_t *dummy = NULL;
        context_switch(&dummy, g_procs[first].context);
    }
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
    proc->context = NULL;
    proc->stack = NULL;
    proc->stack_size = 0;
    
    int next = -1;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i].state == PROC_READY) {
            next = i;
            break;
        }
    }
    
    if (next < 0) {
        for (int i = 0; i < MAX_PROCESSES; ++i) {
            if (i != g_current && g_procs[i].state != PROC_UNUSED) {
                next = i;
                g_procs[next].state = PROC_READY;
                break;
            }
        }
    }
    
    if (next < 0) {
        next = process_create(idle_process, 0);
        if (next < 0) {
            g_scheduler_started = 0;
            g_current = -1;
            return;
        }
    }
    
    int old_current = g_current;
    g_procs[next].state = PROC_RUNNING;
    g_current = next;
    
    process_context_t *dummy = NULL;
    context_switch(&dummy, g_procs[next].context);
}

int get_process_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        if (g_procs[i].state != PROC_UNUSED) {
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
        if (g_procs[i].state != PROC_UNUSED) {
            printk("  Slot %d: PID=%d, State=%d, Context=0x%lx, Stack=0x%lx\n",
                   i, g_procs[i].pid, g_procs[i].state,
                   (unsigned long)g_procs[i].context,
                   (unsigned long)g_procs[i].stack);
        } else {
            printk("  Slot %d: UNUSED\n", i);
        }
    }
}
