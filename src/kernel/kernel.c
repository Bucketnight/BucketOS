#include "bucketos/interrupts.h"
#include "bucketos/kernel.h"
#include "bucketos/memory.h"
#include "bucketos/multiboot.h"
#include "bucketos/ports.h"
#include "bucketos/print.h"
#include "bucketos/shell.h"
#include "bucketos/terminal.h"
#include "bucketos/hypervisor.h"
#include "bucketos/panic.h"
#include "bucketos/serial.h"

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    serial_initialize();
    terminal_initialize();
    print_banner();
    if (serial_is_ready()) {
        print_line("serial: com1 ready");
    }

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("multiboot: invalid loader state");
    }

    print_line("multiboot: ok");
    memory_initialize((const multiboot_info_t *)(uintptr_t)multiboot_info_addr);

    hypervisor_info_t hv = hypervisor_detect();

    print_string("hypervisor: ");
    print_line(hv.name);

    if (hv.present) {
        print_string("vendor id: ");
        print_line(hv.vendor);
    }

    interrupts_initialize();
    shell_initialize();

    __asm__ volatile ("sti");

    shell_prompt();

    for (;;) {
        cpu_halt();
    }
}
