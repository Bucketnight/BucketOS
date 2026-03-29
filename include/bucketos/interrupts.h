// interrupts.h: Interrupt subsystem API (PIC remap, ISR install, enable/disable).

/*
 * Reading guide:
 * - Purpose: interrupts.h: Interrupt subsystem API (PIC remap, ISR install, enable/disable).
 * - Start reading at: interrupts_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_INTERRUPTS_H
#define BUCKETOS_INTERRUPTS_H

#include "bucketos/idt.h"

void interrupts_initialize(void);
void interrupt_dispatch(registers_t *regs);

#endif
