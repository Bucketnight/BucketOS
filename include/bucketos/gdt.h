#ifndef BUCKETOS_GDT_H
#define BUCKETOS_GDT_H

#include "bucketos/common.h"
#include "bucketos/idt.h"

enum {
    GDT_KERNEL_CODE_SELECTOR = 0x08,
    GDT_KERNEL_DATA_SELECTOR = 0x10,
    GDT_USER_CODE_SELECTOR = 0x18,
    GDT_USER_DATA_SELECTOR = 0x20,
    GDT_TSS_SELECTOR = 0x28
};

void gdt_initialize(void);
void tss_set_kernel_stack(uintptr_t stack_top);
uint32_t gdt_enter_user_mode(uintptr_t entry, uintptr_t user_stack);
void gdt_prepare_user_exit(registers_t *regs, uint32_t exit_code);

#endif
