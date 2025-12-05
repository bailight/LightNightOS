#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../lib/stdint.h"
#include "console.h"
#include "timer.h"

#define MAX_PROCESSES     8
#define STACK_SIZE        (4096 * 2)
#define TIME_SLICE_TICKS  10

typedef struct {
    uint64_t r15, r14, r13, r12, rbx, rbp;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) process_context_t;

typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
} proc_state_t;

typedef struct {
    uint64_t            pid;
    proc_state_t        state;
    process_context_t   *context;
    void                *stack;
    uint64_t            stack_size;
} process_t;

void scheduler_init(void);
void scheduler_start(void);
int process_create(void (*entry)(void *), void *arg);
void context_switch(process_context_t **old, process_context_t *new);
void process_yield(void);
void scheduler_tick(void);
uint64_t get_current_pid(void);
void process_exit(int status);
int get_process_count(void);
uint64_t get_process_pid(int index);
proc_state_t get_process_state(int index);
void debug_print_processes(void);

void idle_process(void *arg);

#endif
