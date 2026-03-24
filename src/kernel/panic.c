#include "bucketos/panic.h"
#include "bucketos/ports.h"
#include "bucketos/print.h"
#include "bucketos/terminal.h"

static void panic_stop(void) __attribute__((noreturn));

static void panic_stop(void) {
    __asm__ volatile ("cli");

#if CONFIG_PANIC_HALT
    for (;;) {
        cpu_halt();
    }
#else
    for (;;) {
    }
#endif
}

void panic(const char *message) {
    terminal_set_color(0x0C);
    print_line("");
    print_line("*** kernel panic ***");
    print_line(message);
    terminal_set_color(0x0F);

    panic_stop();
}

void panic_exception(const char *message, const registers_t *regs) {
    terminal_set_color(0x0C);
    print_line("");
    print_line("*** kernel exception ***");
    print_string("reason: ");
    print_line(message);
    print_string("int=");
    print_uint32(regs->int_no);
    print_line("");
    print_string("err=");
    print_hex32(regs->err_code);
    print_line("");
    print_string("eip=");
    print_hex32(regs->eip);
    print_line("");
    terminal_set_color(0x0F);

    panic_stop();
}
