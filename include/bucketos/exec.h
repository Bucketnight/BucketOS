// exec.h: User program loader API (exec from initrd/ramfs into user mapping).

/*
 * Reading guide:
 * - Purpose: exec.h: User program loader API (exec from initrd/ramfs into user mapping).
 * - Start reading at: exec_load
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_EXEC_H
#define BUCKETOS_EXEC_H

#include "bucketos/common.h"

bool exec_load(const char *path);
uint32_t exec_run(const char *path);
uint32_t exec_run_argv(const char *path, int argc, const char *const argv[]);

#endif
