#include "scheduler.h"
#include "console.h"
#include "timer.h"

#define MAX_PROCESSES     8
#define TIME_SLICE_TICKS  10

typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
} proc_state_t;

typedef struct {
    uint64_t    pid;
    proc_state_t state;

    void      (*entry)(void *);
    void       *arg;
} process_t;

static process_t g_procs[MAX_PROCESSES];
static int       g_current = -1;
static uint64_t  g_next_pid = 1;
static unsigned long long g_last_switch_ticks = 0;

void scheduler_init(void) {
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        g_procs[i].pid   = 0;
        g_procs[i].state = PROC_UNUSED;
        g_procs[i].entry = 0;
        g_procs[i].arg   = 0;
    }
    g_current = -1;
    g_next_pid = 1;
    g_last_switch_ticks = timer_ticks();
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
        print_str("scheduler: no free slots\n");
        return -1;
    }

    g_procs[idx].pid   = g_next_pid++;
    g_procs[idx].state = PROC_READY;
    g_procs[idx].entry = entry;
    g_procs[idx].arg   = arg;

    return idx;
}

void process_yield(void) {
}

void scheduler_tick(void) {
    unsigned long long now = timer_ticks();
    if (now - g_last_switch_ticks < TIME_SLICE_TICKS) {
        return;
    }
    g_last_switch_ticks = now;

    if (g_current < 0) {
        for (int i = 0; i < MAX_PROCESSES; ++i) {
            if (g_procs[i].state == PROC_READY) {
                g_current = i;
                g_procs[i].state = PROC_RUNNING;
                g_procs[i].entry(g_procs[i].arg);
            }
        }
        return;
    }

    int next = g_current;
    int found = -1;
    for (int i = 0; i < MAX_PROCESSES; ++i) {
        next = (next + 1) % MAX_PROCESSES;
        if (g_procs[next].state == PROC_READY ||
            g_procs[next].state == PROC_RUNNING) {
            found = next;
            break;
        }
    }

    if (found < 0) {
        return;
    }

    g_current = found;
    g_procs[found].state = PROC_RUNNING;
    g_procs[found].entry(g_procs[found].arg);
}

