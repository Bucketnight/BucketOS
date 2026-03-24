#ifndef BUCKETOS_RAMFS_H
#define BUCKETOS_RAMFS_H

#include "bucketos/common.h"

typedef void (*ramfs_list_callback_t)(const char *name, bool is_dir, void *context);

void ramfs_initialize(void);
void ramfs_load_initrd(const void *address, size_t size);
bool ramfs_list(const char *path, ramfs_list_callback_t callback, void *context);
bool ramfs_is_dir(const char *path);
bool ramfs_read(const char *path, const char **data, size_t *size);
bool ramfs_mkdir(const char *path);
bool ramfs_touch(const char *path);
bool ramfs_write(const char *path, const char *data);

#endif
