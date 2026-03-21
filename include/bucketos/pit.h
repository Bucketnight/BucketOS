#ifndef BUCKETKERNEL_PIT_H
#define BUCKETKERNEL_PIT_H

#include "bucketkernel/common.h"

void pit_initialize(uint32_t frequency_hz);
void pit_handle_tick(void);
uint32_t pit_ticks(void);

#endif
