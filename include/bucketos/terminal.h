#ifndef BUCKETOS_TERMINAL_H
#define BUCKETOS_TERMINAL_H

#include "bucketos/common.h"
#include "bucketos/framebuffer.h"

void terminal_configure_framebuffer(const framebuffer_info_t *framebuffer);
void terminal_initialize(void);
void terminal_clear(void);
void terminal_put_char(char c);
void terminal_write(const char *data);
void terminal_write_line(const char *data);
void terminal_set_color(uint8_t color);
uint8_t terminal_get_color(void);

#endif
