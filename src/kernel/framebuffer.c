#include "bucketos/framebuffer.h"

static framebuffer_info_t g_framebuffer;

void framebuffer_initialize(const multiboot_info_t *mbi) {
    g_framebuffer.available = false;
    g_framebuffer.address = 0;
    g_framebuffer.pitch = 0;
    g_framebuffer.width = 0;
    g_framebuffer.height = 0;
    g_framebuffer.bpp = 0;
    g_framebuffer.type = 0;

    if ((mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER) == 0u) {
        return;
    }

    g_framebuffer.available = true;
    g_framebuffer.address = (uintptr_t)mbi->framebuffer_addr;
    g_framebuffer.pitch = mbi->framebuffer_pitch;
    g_framebuffer.width = mbi->framebuffer_width;
    g_framebuffer.height = mbi->framebuffer_height;
    g_framebuffer.bpp = mbi->framebuffer_bpp;
    g_framebuffer.type = mbi->framebuffer_type;
}

const framebuffer_info_t *framebuffer_info(void) {
    return &g_framebuffer;
}
