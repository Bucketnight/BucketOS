#ifndef BUCKETOS_EXEC_H
#define BUCKETOS_EXEC_H

#include "bucketos/common.h"

bool exec_load(const char *path);
uint32_t exec_run(const char *path);
uint32_t exec_run_argv(const char *path, int argc, const char *const argv[]);

#endif
