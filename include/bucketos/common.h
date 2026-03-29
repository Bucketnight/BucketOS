// common.h: Common kernel types, macros, and small utilities.

/*
 * Reading guide:
 * - Purpose: common.h: Common kernel types, macros, and small utilities.
 * - Start reading at: (top of file)
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_COMMON_H
#define BUCKETOS_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ARRAY_SIZE(value) (sizeof(value) / sizeof((value)[0]))

#endif
