#include "bucketos/config.h"
#include "bucketos/panic.h"
#include "bucketos/ports.h"
#include "bucketos/print.h"
#include "bucketos/terminal.h"
#include "bucketos/pit.h"

static void panic_delay_ticks(uint32_t ticks_to_wait) {
    const uint32_t start = pit_ticks();

    while ((pit_ticks() - start) < ticks_to_wait)
    {
        __asm__ volatile ("hlt");
    }
    
}

static void panic_stop(void) __attribute__((noreturn));

static void panic_stop(void) {
    __asm__ volatile ("cli");

    terminal_set_color(0x0C);
    print_line("rebooting in 5 seconds...");
    terminal_set_color(0x0F);

    __asm__ volatile ("sti");
    panic_delay_ticks(500);
    __asm__ volatile ("cli");

    cpu_reboot();
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
