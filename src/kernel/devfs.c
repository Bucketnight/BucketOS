// devfs.c: devfs implementation (mountable device filesystem).

/*
 * Reading guide:
 * - Purpose: devfs.c: devfs implementation (mountable device filesystem).
 * - Start reading at: devfs_initialize
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/devfs.h"
#include "bucketos/print.h"
#include "bucketos/string.h"

typedef struct {
    const char *name;
} devfs_entry_t;

static const devfs_entry_t g_entries[] = {
    { "null" },
    { "console" },
    { "fb0" },
    { "mouse0" }
};

void devfs_initialize(void) {
}

static bool devfs_is_root(const char *path) {
    return strcmp(path, "/") == 0 || path[0] == '\0';
}

static const char *devfs_name(const char *path) {
    return path[0] == '/' ? path + 1 : path;
}

bool devfs_list(const char *path, vfs_list_callback_t callback, void *context) {
    if (!devfs_is_root(path)) {
        return false;
    }

    for (size_t i = 0; i < ARRAY_SIZE(g_entries); ++i) {
        callback(g_entries[i].name, false, context);
    }
    return true;
}

bool devfs_is_dir(const char *path) {
    return devfs_is_root(path);
}

bool devfs_read(const char *path, const char **data, size_t *size) {
    const char *name = devfs_name(path);

    if (strcmp(name, "null") == 0) {
        *data = "";
        *size = 0;
        return true;
    }

    if (strcmp(name, "console") == 0) {
        *data = "console device";
        *size = strlen(*data);
        return true;
    }

    if (strcmp(name, "fb0") == 0) {
        *data = "framebuffer device";
        *size = strlen(*data);
        return true;
    }

    if (strcmp(name, "mouse0") == 0) {
        *data = "mouse device";
        *size = strlen(*data);
        return true;
    }

    return false;
}

bool devfs_touch(const char *path) {
    (void)path;
    return false;
}

bool devfs_write(const char *path, const char *data) {
    const char *name = devfs_name(path);

    if (strcmp(name, "null") == 0) {
        return true;
    }

    if (strcmp(name, "console") == 0) {
        print_line(data);
        return true;
    }

    return false;
}
