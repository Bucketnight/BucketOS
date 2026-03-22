#include "bucketos/interrupts.h"
#include "bucketos/kernel.h"
#include "bucketos/memory.h"
#include "bucketos/multiboot.h"
#include "bucketos/ports.h"
#include "bucketos/print.h"
#include "bucketos/shell.h"
#include "bucketos/terminal.h"
#include "bucketos/hypervisor.h"

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    terminal_initialize();
    print_banner();

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        print_line("multiboot: invalid loader state");
        for (;;) {
            cpu_halt();
        }
    }

    print_line("multiboot: ok");
    memory_initialize((const multiboot_info_t *)(uintptr_t)multiboot_info_addr);

    hypervisor_info_t hv = hypervisor_detect();
    if (!hv.present) {
        print_line("hypervisor: bare metal");
    } else {
        print_string("hypervisor: ");
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

