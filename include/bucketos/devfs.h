#ifndef BUCKETOS_DEVFS_H
#define BUCKETOS_DEVFS_H

#include "bucketos/common.h"
#include "bucketos/vfs.h"

void devfs_initialize(void);
bool devfs_list(const char *path, vfs_list_callback_t callback, void *context);
bool devfs_is_dir(const char *path);
bool devfs_read(const char *path, const char **data, size_t *size);
bool devfs_touch(const char *path);
bool devfs_write(const char *path, const char *data);

#endif
    
