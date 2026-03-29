// vfs.h: Minimal VFS layer API (mount points + path traversal + list/read).

/*
 * Reading guide:
 * - Purpose: vfs.h: Minimal VFS layer API (mount points + path traversal + list/read).
 * - Start reading at: void
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_VFS_H
#define BUCKETOS_VFS_H

#include "bucketos/common.h"

typedef void (*vfs_list_callback_t)(const char *name, bool is_dir, void *context);

void vfs_initialize(void);
void vfs_load_initrd(const void *address, size_t size);
bool vfs_list(const char *path, vfs_list_callback_t callback, void *context);
bool vfs_is_dir(const char *path);
bool vfs_read(const char *path, const char **data, size_t *size);
bool vfs_mkdir(const char *path);
bool vfs_touch(const char *path);
bool vfs_write(const char *path, const char *data);

#endif
