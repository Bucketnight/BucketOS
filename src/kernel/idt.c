// idt.c: IDT descriptor table setup and ISR gate registration.

/*
 * Reading guide:
 * - Purpose: idt.c: IDT descriptor table setup and ISR gate registration.
 * - Start reading at: idt_set_gate
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/idt.h"
#include "bucketos/ports.h"
#include "bucketos/string.h"

typedef struct {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t g_idt[256];
static idt_ptr_t g_idt_ptr;

void idt_set_gate(uint8_t vector, uintptr_t handler, uint16_t selector, uint8_t flags) {
    g_idt[vector].base_low = (uint16_t)(handler & 0xFFFFu);
    g_idt[vector].selector = selector;
    g_idt[vector].zero = 0;
    g_idt[vector].flags = flags;
    g_idt[vector].base_high = (uint16_t)((handler >> 16) & 0xFFFFu);
}

void idt_initialize(void) {
    memset(g_idt, 0, sizeof(g_idt));
    g_idt_ptr.limit = (uint16_t)(sizeof(g_idt) - 1);
    g_idt_ptr.base = (uintptr_t)&g_idt[0];

    __asm__ volatile ("lidt %0" : : "m"(g_idt_ptr));
}
