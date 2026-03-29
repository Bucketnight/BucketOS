// console.h: Kernel console device API (read/write/clear; backs /dev/console).

/*
 * Reading guide:
 * - Purpose: console.h: Kernel console device API (read/write/clear; backs /dev/console).
 * - Start reading at: console_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

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
