#include "bucketos/interrupts.h"
#include "bucketos/kernel.h"
#include "bucketos/memory.h"
#include "bucketos/multiboot.h"
#include "bucketos/ports.h"
#include "bucketos/print.h"
#include "bucketos/shell.h"
#include "bucketos/terminal.h"
#include "bucketos/framebuffer.h"
#include "bucketos/vfs.h"
#include "bucketos/hypervisor.h"
#include "bucketos/panic.h"
#include "bucketos/serial.h"

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    const multiboot_info_t *multiboot =
        (const multiboot_info_t *)(uintptr_t)multiboot_info_addr;

    serial_initialize();
    terminal_initialize();
    print_logo();
    print_banner();
    if (serial_is_ready()) {
        print_line("serial: com1 ready");
    }

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("multiboot: invalid loader state");
    }

    print_line("multiboot: ok");
    memory_initialize(multiboot);
    vfs_initialize();

    if ((multiboot->flags & MULTIBOOT_INFO_MODS) != 0u && multiboot->mods_count > 0u) {
        const multiboot_module_t *module =
            (const multiboot_module_t *)(uintptr_t)multiboot->mods_addr;
        const size_t initrd_size = (size_t)(module[0].mod_end - module[0].mod_start);
        vfs_load_initrd((const void *)(uintptr_t)module[0].mod_start, initrd_size);
        print_line("initrd: loaded");
    } else {
        print_line("initrd: unavailable");
    }

    framebuffer_initialize(multiboot);

    const framebuffer_info_t *framebuffer = framebuffer_info();
    if (framebuffer->available) {
        print_string("framebuffer: ");
        print_uint32(framebuffer->width);
        print_char('x');
        print_uint32(framebuffer->height);
        print_char('x');
        print_uint32(framebuffer->bpp);
        print_line("");
        print_string("framebuffer type: ");
        print_uint32(framebuffer->type);
        print_line("");
        print_string("framebuffer pitch: ");
        print_uint32(framebuffer->pitch);
        print_line("");
        print_string("framebuffer addr: ");
        print_hex32((uint32_t)framebuffer->address);
        print_line("");
    } else {
        print_line("framebuffer: unavailable");
    }

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
        if (shell_has_pending_command()) {
            shell_run_pending_command();
            continue;
        }
        cpu_halt();
    }
}
