// syscall.h: Kernel syscall numbers and ABI constants shared with userspace.

/*
 * Reading guide:
 * - Purpose: syscall.h: Kernel syscall numbers and ABI constants shared with userspace.
 * - Start reading at: syscall_dispatch
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_SYSCALL_H
#define BUCKETOS_SYSCALL_H

#include "bucketos/idt.h"

enum {
    SYSCALL_WRITE = 1,
    SYSCALL_EXIT = 2,
    SYSCALL_OPEN = 3,
    SYSCALL_READ = 4,
    SYSCALL_CLOSE = 5,
    SYSCALL_YIELD = 6,
    SYSCALL_LIST = 7,
    SYSCALL_CLEAR = 8,
    SYSCALL_FB_INFO = 9,
    SYSCALL_FB_PUTPIXEL = 10,
    SYSCALL_FB_FILL_RECT = 11,
    SYSCALL_FB_BLIT = 12
};

uint32_t syscall_dispatch(registers_t *regs);
void syscall_reset_process(void);

#endif
