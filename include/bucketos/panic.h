// panic.h: Kernel panic API (fatal error reporting and halt/reboot policy).

/*
 * Reading guide:
 * - Purpose: panic.h: Kernel panic API (fatal error reporting and halt/reboot policy).
 * - Start reading at: panic
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_PANIC_H
#define BUCKETOS_PANIC_H

#include "bucketos/idt.h"

void panic(const char *message) __attribute__((noreturn));
void panic_exception(const char *message, const registers_t *regs) __attribute__((noreturn));

#endif
