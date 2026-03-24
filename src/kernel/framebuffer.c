#include "bucketos/framebuffer.h"
#include "bucketos/string.h"

static framebuffer_info_t g_framebuffer;

static bool framebuffer_can_draw(void) {
    return g_framebuffer.available
        && g_framebuffer.address != 0
        && g_framebuffer.type == 1u
        && (g_framebuffer.bpp == 24u || g_framebuffer.bpp == 32u);
}

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

void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!framebuffer_can_draw()) {
        return;
    }

    if (x >= g_framebuffer.width || y >= g_framebuffer.height) {
        return;
    }

    uint8_t *const base = (uint8_t *)g_framebuffer.address;
    uint8_t *const pixel =
        base + y * g_framebuffer.pitch + x * (g_framebuffer.bpp / 8u);

    pixel[0] = (uint8_t)(color & 0xFFu);
    pixel[1] = (uint8_t)((color >> 8) & 0xFFu);
    pixel[2] = (uint8_t)((color >> 16) & 0xFFu);

    if (g_framebuffer.bpp == 32u) {
        pixel[3] = (uint8_t)((color >> 24) & 0xFFu);
    }
}

void framebuffer_fill(uint32_t color) {
    if (!framebuffer_can_draw()) {
        return;
    }

    for (uint32_t y = 0; y < g_framebuffer.height; ++y) {
        for (uint32_t x = 0; x < g_framebuffer.width; ++x) {
            framebuffer_put_pixel(x, y, color);
        }
    }
}
void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    for (uint32_t row = 0; row < height; ++row) {
        for (uint32_t col = 0; col < width; ++col) {
            framebuffer_put_pixel(x + col, y + row, color);
        }
    }
}
