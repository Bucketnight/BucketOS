#include "bucketos/gdt.h"
#include "bucketos/syscall.h"
#include "bucketos/print.h"

static uint32_t syscall_write(registers_t *regs) {
    const char *const text = (const char *)(uintptr_t)regs->ebx;
    const size_t length = (size_t)regs->ecx;

    if (text == 0) {
        return (uint32_t)-1;
    }

    for (size_t i = 0; i < length; ++i) {
        print_char(text[i]);
    }

    return (uint32_t)length;
}

static uint32_t syscall_exit(registers_t *regs) {
    gdt_prepare_user_exit(regs, regs->ebx);
    return regs->ebx;
}

uint32_t syscall_dispatch(registers_t *regs) {
    switch (regs->eax) {
        case SYSCALL_WRITE:
            return syscall_write(regs);
        case SYSCALL_EXIT:
            return syscall_exit(regs);
        default:
            return (uint32_t)-1;
    }
}
