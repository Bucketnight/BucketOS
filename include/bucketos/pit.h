// pit.h: PIT timer API (tick counter).

/*
 * Reading guide:
 * - Purpose: pit.h: PIT timer API (tick counter).
 * - Start reading at: pit_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_PIT_H
#define BUCKETOS_PIT_H

#include "bucketos/common.h"

void pit_initialize(uint32_t frequency_hz);
void pit_handle_tick(void);
uint32_t pit_ticks(void);

#endif
