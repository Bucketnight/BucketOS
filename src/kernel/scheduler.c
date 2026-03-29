// scheduler.c: Minimal scheduler implementation used by sys_yield and exit.

/*
 * Reading guide:
 * - Purpose: scheduler.c: Minimal scheduler implementation used by sys_yield and exit.
 * - Start reading at: scheduler_initialize
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/scheduler.h"

enum {
    SCHEDULER_PROCESS_MAX = 4
};

static process_t *g_processes[SCHEDULER_PROCESS_MAX];
static size_t g_process_count;
static size_t g_scheduler_index;
static process_t *g_scheduler_current;

void scheduler_initialize(void) {
    g_process_count = 0;
    g_scheduler_index = 0;
    g_scheduler_current = 0;
}

void scheduler_register(process_t *process) {
    if (process == 0) {
        return;
    }

    for (size_t index = 0; index < g_process_count; ++index) {
        if (g_processes[index] == process) {
            return;
        }
    }

    if (g_process_count < SCHEDULER_PROCESS_MAX) {
        g_processes[g_process_count++] = process;
    }

    if (g_scheduler_current == 0) {
        g_scheduler_index = 0;
        g_scheduler_current = process;
    }
}

process_t *scheduler_current(void) {
    return g_scheduler_current;
}

void scheduler_set_current(process_t *process) {
    g_scheduler_current = process;

    for (size_t index = 0; index < g_process_count; ++index) {
        if (g_processes[index] == process) {
            g_scheduler_index = index;
            break;
        }
    }
}

process_t *scheduler_yield(void) {
    if (g_process_count == 0 || g_scheduler_current == 0) {
        return g_scheduler_current;
    }

    for (size_t step = 1; step <= g_process_count; ++step) {
        const size_t index = (g_scheduler_index + step) % g_process_count;
        process_t *const candidate = g_processes[index];

        if (candidate != 0
            && candidate->state != PROCESS_STATE_UNUSED
            && candidate->state != PROCESS_STATE_EXITED) {
            if (g_scheduler_current->state == PROCESS_STATE_RUNNING) {
                g_scheduler_current->state = PROCESS_STATE_READY;
            }

            g_scheduler_index = index;
            g_scheduler_current = candidate;
            if (g_scheduler_current->state == PROCESS_STATE_READY) {
                g_scheduler_current->state = PROCESS_STATE_RUNNING;
            }
            break;
        }
    }

    return g_scheduler_current;
}
