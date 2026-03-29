// kernel.h: Kernel entry declarations and shared kernel-wide definitions.

/*
 * Reading guide:
 * - Purpose: kernel.h: Kernel entry declarations and shared kernel-wide definitions.
 * - Start reading at: kernel_main
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_KERNEL_H
#define BUCKETOS_KERNEL_H

#include "bucketos/common.h"

void kernel_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr);

#endif
