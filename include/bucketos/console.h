#ifndef BUCKETOS_CONSOLE_H
#define BUCKETOS_CONSOLE_H

#include "bucketos/common.h"

void console_initialize(void);
bool console_input_is_active(void);
void console_push_input(char c);
size_t console_read(char *buffer, size_t size);
void console_write(const char *data, size_t size);
void console_clear(void);

#endif
