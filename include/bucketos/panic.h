#ifndef BUCKETOS_PANIC_H
#define BUCKETOS_PANIC_H

#include "bucketos/idt.h"

void panic(const char *message) __attribute__((noreturn));
void panic_exception(const char *message, const registers_t *regs) __attribute__((noreturn));

#endif
