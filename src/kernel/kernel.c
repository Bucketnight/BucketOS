#include "bucketkernel/interrupts.h"
#include "bucketkernel/kernel.h"
#include "bucketkernel/memory.h"
#include "bucketkernel/multiboot.h"
#include "bucketkernel/ports.h"
#include "bucketkernel/print.h"
#include "bucketkernel/shell.h"
#include "bucketkernel/terminal.h"
#include "bucketkernel/hypervisor.h"

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

