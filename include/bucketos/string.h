// string.h: Tiny libc-like string/memory routines used in freestanding code.

/*
 * Reading guide:
 * - Purpose: string.h: Tiny libc-like string/memory routines used in freestanding code.
 * - Start reading at: strlen
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_STRING_H
#define BUCKETOS_STRING_H

#include "bucketos/common.h"

void *memcpy(void *dest, const void *src, size_t count);
void *memset(void *dest, int value, size_t count);
size_t strlen(const char *str);
int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, size_t count);

#endif
