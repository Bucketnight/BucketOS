// scheduler.h: Minimal scheduler API (yield/round-robin bookkeeping).

/*
 * Reading guide:
 * - Purpose: scheduler.h: Minimal scheduler API (yield/round-robin bookkeeping).
 * - Start reading at: scheduler_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_SCHEDULER_H
#define BUCKETOS_SCHEDULER_H

#include "bucketos/common.h"
#include "bucketos/process.h"

void scheduler_initialize(void);
void scheduler_register(process_t *process);
process_t *scheduler_current(void);
void scheduler_set_current(process_t *process);
process_t *scheduler_yield(void);

#endif
