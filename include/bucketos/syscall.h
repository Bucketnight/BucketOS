#ifndef BUCKETOS_SYSCALL_H
#define BUCKETOS_SYSCALL_H

#include "bucketos/idt.h"

enum {
    SYSCALL_WRITE = 1,
    SYSCALL_EXIT = 2
};

uint32_t syscall_dispatch(registers_t *regs);

#endif
