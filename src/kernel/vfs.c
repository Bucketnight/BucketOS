#include "bucketos/devfs.h"
#include "bucketos/ramfs.h"
#include "bucketos/string.h"
#include "bucketos/vfs.h"

static bool vfs_is_dev_path(const char *path) {
    return strcmp(path, "/dev") == 0 || strncmp(path, "/dev/", 5) == 0;
}

static const char *vfs_dev_relative(const char *path) {
    return strcmp(path, "/dev") == 0 ? "/" : path + 4;
}

void vfs_initialize(void) {
    ramfs_initialize();
    ramfs_mkdir("/dev");
    devfs_initialize();
}

void vfs_load_initrd(const void *address, size_t size) {
    ramfs_load_initrd(address, size);
    ramfs_mkdir("/dev");
}

bool vfs_list(const char *path, vfs_list_callback_t callback, void *context) {
    return vfs_is_dev_path(path)
        ? devfs_list(vfs_dev_relative(path), callback, context)
        : ramfs_list(path, callback, context);
}

bool vfs_is_dir(const char *path) {
    return vfs_is_dev_path(path)
        ? devfs_is_dir(vfs_dev_relative(path))
        : ramfs_is_dir(path);
}

bool vfs_read(const char *path, const char **data, size_t *size) {
    return vfs_is_dev_path(path)
        ? devfs_read(vfs_dev_relative(path), data, size)
        : ramfs_read(path, data, size);
}

bool vfs_mkdir(const char *path) {
    return vfs_is_dev_path(path) ? false : ramfs_mkdir(path);
}

bool vfs_touch(const char *path) {
    return vfs_is_dev_path(path)
        ? devfs_touch(vfs_dev_relative(path))
        : ramfs_touch(path);
}

bool vfs_write(const char *path, const char *data) {
    return vfs_is_dev_path(path)
        ? devfs_write(vfs_dev_relative(path), data)
        : ramfs_write(path, data);
}
