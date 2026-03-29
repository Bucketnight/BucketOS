// idt.h: IDT setup API.

/*
 * Reading guide:
 * - Purpose: idt.h: IDT setup API.
 * - Start reading at: idt_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_IDT_H
#define BUCKETOS_IDT_H

#include "bucketos/common.h"

typedef struct {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t int_no;
    uint32_t err_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} registers_t;

void idt_initialize(void);
void idt_set_gate(uint8_t vector, uintptr_t handler, uint16_t selector, uint8_t flags);

#endif
