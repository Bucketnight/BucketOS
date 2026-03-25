#include "bucketos/gdt.h"
#include "bucketos/interrupts.h"
#include "bucketos/keyboard.h"
#include "bucketos/panic.h"
#include "bucketos/pit.h"
#include "bucketos/ports.h"
#include "bucketos/print.h"
#include "bucketos/syscall.h"
#include "bucketos/terminal.h"

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void isr32(void);
extern void isr33(void);
extern void isr34(void);
extern void isr35(void);
extern void isr36(void);
extern void isr37(void);
extern void isr38(void);
extern void isr39(void);
extern void isr40(void);
extern void isr41(void);
extern void isr42(void);
extern void isr43(void);
extern void isr44(void);
extern void isr45(void);
extern void isr46(void);
extern void isr47(void);
extern void isr128(void);

static const char *const g_exception_names[] = {
    "divide error",
    "debug",
    "non-maskable interrupt",
    "breakpoint",
    "overflow",
    "bound range exceeded",
    "invalid opcode",
    "device not available",
    "double fault",
    "coprocessor segment overrun",
    "invalid tss",
    "segment not present",
    "stack segment fault",
    "general protection fault",
    "page fault",
    "reserved",
    "x87 floating point",
    "alignment check",
    "machine check",
    "simd floating point",
    "virtualization",
    "control protection",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "hypervisor injection",
    "vmm communication",
    "security",
    "reserved"
};

static void pic_remap(void) {
    const uint8_t master_mask = inb(0x21);
    const uint8_t slave_mask = inb(0xA1);

    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();

    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    outb(0x21, master_mask & (uint8_t)~0x03u);
    outb(0xA1, slave_mask);
}

static void pic_send_eoi(uint32_t interrupt_number) {
    if (interrupt_number >= 40) {
        outb(0xA0, 0x20);
    }

    outb(0x20, 0x20);
}

void interrupts_initialize(void) {
    static void (*const handlers[48])(void) = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
        isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
        isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
        isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
    };

    idt_initialize();
    pic_remap();

    for (uint8_t vector = 0; vector < 48; ++vector) {
        idt_set_gate(vector, (uintptr_t)handlers[vector], GDT_KERNEL_CODE_SELECTOR, 0x8E);
    }
    idt_set_gate(0x80u, (uintptr_t)isr128, GDT_KERNEL_CODE_SELECTOR, 0xEE);

    pit_initialize(100);
    keyboard_initialize();

    print_line("gdt: loaded");
    print_line("idt: loaded");
    print_line("pic: remapped");
    print_line("pit: 100 hz");
    print_line("keyboard: irq1 online");
}

void interrupt_dispatch(registers_t *regs) {
    if (regs->int_no < 32) {
        panic_exception(g_exception_names[regs->int_no], regs);
    }

    if (regs->int_no == 0x80u) {
        regs->eax = syscall_dispatch(regs);
        return;
    }

    switch (regs->int_no) {
        case 32:
            pit_handle_tick();
            break;
        case 33:
            keyboard_handle_irq();
            break;
        default:
            break;
    }

    if (regs->int_no >= 32 && regs->int_no < 48) {
        pic_send_eoi(regs->int_no);
    }
}
