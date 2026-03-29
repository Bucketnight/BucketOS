// gdt.c: GDT + TSS implementation (segment descriptors and ring 3 transitions).

/*
 * Reading guide:
 * - Purpose: gdt.c: GDT + TSS implementation (segment descriptors and ring 3 transitions).
 * - Start reading at: tss_set_kernel_stack
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/gdt.h"

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed)) gdt_ptr_t;

typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt_selector;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

extern char stack_top;

static gdt_entry_t g_gdt[6];
static gdt_ptr_t g_gdt_ptr;
static tss_t g_tss;
static volatile uint32_t g_user_exit_code;

static void gdt_user_mode_enter_asm(uintptr_t entry, uintptr_t user_stack)
    __attribute__((noreturn, naked));
static void gdt_user_mode_enter_asm(uintptr_t entry, uintptr_t user_stack) {
    (void)entry;
    (void)user_stack;

    __asm__ volatile (
        "cli\n"
        "movl 8(%%esp), %%edx\n"
        "movl 4(%%esp), %%ecx\n"
        "movw %0, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "pushl %0\n"
        "pushl %%edx\n"
        "pushfl\n"
        "orl $0x200, (%%esp)\n"
        "pushl %1\n"
        "pushl %%ecx\n"
        "iret\n"
        :
        : "i"(GDT_USER_DATA_SELECTOR | 0x03u),
          "i"(GDT_USER_CODE_SELECTOR | 0x03u)
        : "ax", "ecx", "edx", "memory");

    __builtin_unreachable();
}

static void gdt_user_mode_return(void) __attribute__((naked));
static void gdt_user_mode_return(void) {
    __asm__ volatile (
        "addl $8, %esp\n"
        "ret\n");
}

static void gdt_set_entry(
    uint32_t index, uintptr_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
    g_gdt[index].limit_low = (uint16_t)(limit & 0xFFFFu);
    g_gdt[index].base_low = (uint16_t)(base & 0xFFFFu);
    g_gdt[index].base_mid = (uint8_t)((base >> 16) & 0xFFu);
    g_gdt[index].access = access;
    g_gdt[index].granularity = (uint8_t)(((limit >> 16) & 0x0Fu) | (granularity & 0xF0u));
    g_gdt[index].base_high = (uint8_t)((base >> 24) & 0xFFu);
}

static void tss_initialize(uintptr_t stack_top_addr) {
    gdt_set_entry(5, (uintptr_t)&g_tss, (uint32_t)(sizeof(g_tss) - 1u), 0x89, 0x00);

    g_tss.prev_tss = 0;
    g_tss.esp0 = (uint32_t)stack_top_addr;
    g_tss.ss0 = GDT_KERNEL_DATA_SELECTOR;
    g_tss.esp1 = 0;
    g_tss.ss1 = 0;
    g_tss.esp2 = 0;
    g_tss.ss2 = 0;
    g_tss.cr3 = 0;
    g_tss.eip = 0;
    g_tss.eflags = 0;
    g_tss.eax = 0;
    g_tss.ecx = 0;
    g_tss.edx = 0;
    g_tss.ebx = 0;
    g_tss.esp = 0;
    g_tss.ebp = 0;
    g_tss.esi = 0;
    g_tss.edi = 0;
    g_tss.es = GDT_USER_DATA_SELECTOR | 0x03u;
    g_tss.cs = GDT_USER_CODE_SELECTOR | 0x03u;
    g_tss.ss = GDT_USER_DATA_SELECTOR | 0x03u;
    g_tss.ds = GDT_USER_DATA_SELECTOR | 0x03u;
    g_tss.fs = GDT_USER_DATA_SELECTOR | 0x03u;
    g_tss.gs = GDT_USER_DATA_SELECTOR | 0x03u;
    g_tss.ldt_selector = 0;
    g_tss.trap = 0;
    g_tss.iomap_base = sizeof(g_tss);
}

void tss_set_kernel_stack(uintptr_t stack_top_addr) {
    g_tss.esp0 = (uint32_t)stack_top_addr;
}

uint32_t gdt_enter_user_mode(uintptr_t entry, uintptr_t user_stack) {
    uintptr_t kernel_stack;

    __asm__ volatile ("mov %%esp, %0" : "=r"(kernel_stack));
    tss_set_kernel_stack(kernel_stack - sizeof(uint32_t));
    g_user_exit_code = 0xFFFFFFFFu;
    gdt_user_mode_enter_asm(entry, user_stack);
    return g_user_exit_code;
}

void gdt_prepare_user_exit(registers_t *regs, uint32_t exit_code) {
    g_user_exit_code = exit_code;
    regs->gs = GDT_KERNEL_DATA_SELECTOR;
    regs->fs = GDT_KERNEL_DATA_SELECTOR;
    regs->es = GDT_KERNEL_DATA_SELECTOR;
    regs->ds = GDT_KERNEL_DATA_SELECTOR;
    regs->eip = (uint32_t)(uintptr_t)gdt_user_mode_return;
    regs->cs = GDT_KERNEL_CODE_SELECTOR;
    regs->eflags |= 0x200u;
}

void gdt_initialize(void) {
    g_gdt_ptr.limit = (uint16_t)(sizeof(g_gdt) - 1u);
    g_gdt_ptr.base = (uintptr_t)&g_gdt[0];

    gdt_set_entry(0, 0, 0, 0, 0);
    gdt_set_entry(1, 0, 0x000FFFFFu, 0x9Au, 0xCFu);
    gdt_set_entry(2, 0, 0x000FFFFFu, 0x92u, 0xCFu);
    gdt_set_entry(3, 0, 0x000FFFFFu, 0xFAu, 0xCFu);
    gdt_set_entry(4, 0, 0x000FFFFFu, 0xF2u, 0xCFu);
    tss_initialize((uintptr_t)&stack_top);

    __asm__ volatile ("lgdt %0" : : "m"(g_gdt_ptr));
    __asm__ volatile (
        "ljmp %0, $1f\n"
        "1:\n"
        :
        : "i"(GDT_KERNEL_CODE_SELECTOR)
        : "memory");
    __asm__ volatile (
        "movw %0, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        "movw %%ax, %%ss\n"
        :
        : "i"(GDT_KERNEL_DATA_SELECTOR)
        : "ax", "memory");
    __asm__ volatile ("ltr %0" : : "r"((uint16_t)GDT_TSS_SELECTOR) : "memory");
}
