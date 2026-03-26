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
