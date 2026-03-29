// print.h: Kernel printing helpers (strings/ints/hex; used by early boot + shell).

/*
 * Reading guide:
 * - Purpose: print.h: Kernel printing helpers (strings/ints/hex; used by early boot + shell).
 * - Start reading at: print_char
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_PRINT_H
#define BUCKETOS_PRINT_H

#include "bucketos/common.h"

void print_char(char c);
void print_string(const char *text);
void print_line(const char *text);
void print_uint32(uint32_t value);
void print_hex32(uint32_t value);
void print_hex64(uint64_t value);
void print_banner(void);
void print_logo(void);

#endif
