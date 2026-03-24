#ifndef BUCKETOS_FRAMEBUFFER_H
#define BUCKETOS_FRAMEBUFFER_H

#include "bucketos/common.h"
#include "bucketos/multiboot.h"

typedef struct {
    bool available;
    uintptr_t address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type;
} framebuffer_info_t;

void framebuffer_initialize(const multiboot_info_t *mbi);
const framebuffer_info_t *framebuffer_info(void);
void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void framebuffer_fill(uint32_t color);
void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);


#endif
