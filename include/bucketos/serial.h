// serial.h: Serial logging API (COM1 init + write).

/*
 * Reading guide:
 * - Purpose: serial.h: Serial logging API (COM1 init + write).
 * - Start reading at: serial_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_SERIAL_H
#define BUCKETOS_SERIAL_H

#include "bucketos/common.h"

void serial_initialize(void);
bool serial_is_ready(void);
void serial_write_char(char c);
void serial_write(const char *text);
void serial_write_line(const char *text);

#endif
