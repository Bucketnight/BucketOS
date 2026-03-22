#ifndef BUCKETKERNEL_INTERRUPTS_H
#define BUCKETKERNEL_INTERRUPTS_H

#include "bucketos/idt.h"

void interrupts_initialize(void);
void interrupt_dispatch(registers_t *regs);

#endif
