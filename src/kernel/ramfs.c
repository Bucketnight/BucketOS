#include "bucketos/ramfs.h"
#include "bucketos/memory.h"
#include "bucketos/string.h"

enum {
    RAMFS_NODE_NAME_MAX = 31
};

typedef struct ramfs_node {
    bool is_dir;
    char name[RAMFS_NODE_NAME_MAX + 1];
    struct ramfs_node *parent;
    struct ramfs_node *first_child;
    struct ramfs_node *next_sibling;
    char *data;
    size_t size;
} ramfs_node_t;

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} __attribute__((packed)) tar_header_t;

static ramfs_node_t *g_root;

static size_t segment_length(const char *path) {
    size_t length = 0;

    while (path[length] != '\0' && path[length] != '/') {
        ++length;
    }

    return length;
}

static bool names_equal(const char *name, const char *segment, size_t segment_len) {
    return strlen(name) == segment_len && strncmp(name, segment, segment_len) == 0;
}

static ramfs_node_t *ramfs_alloc_node(const char *name, bool is_dir) {
    ramfs_node_t *node = (ramfs_node_t *)kmalloc(sizeof(ramfs_node_t));
    if (node == 0) {
        return 0;
    }

    memset(node, 0, sizeof(*node));
    node->is_dir = is_dir;

    size_t length = strlen(name);
    if (length > RAMFS_NODE_NAME_MAX) {
        length = RAMFS_NODE_NAME_MAX;
    }

    memcpy(node->name, name, length);
    node->name[length] = '\0';
    return node;
}

static ramfs_node_t *ramfs_find_child(ramfs_node_t *parent, const char *name, size_t name_len) {
    for (ramfs_node_t *child = parent->first_child; child != 0; child = child->next_sibling) {
        if (names_equal(child->name, name, name_len)) {
            return child;
        }
    }

    return 0;
}

static const ramfs_node_t *ramfs_find_child_const(const ramfs_node_t *parent, const char *name, size_t name_len) {
    for (const ramfs_node_t *child = parent->first_child; child != 0; child = child->next_sibling) {
        if (names_equal(child->name, name, name_len)) {
            return child;
        }
    }

    return 0;
}

static ramfs_node_t *ramfs_add_child(ramfs_node_t *parent, const char *name, bool is_dir) {
    ramfs_node_t *node = ramfs_alloc_node(name, is_dir);
    if (node == 0) {
        return 0;
    }

    node->parent = parent;
    node->next_sibling = parent->first_child;
    parent->first_child = node;
    return node;
}

static const ramfs_node_t *ramfs_resolve_path(const char *path) {
    if (g_root == 0 || path == 0 || path[0] != '/') {
        return 0;
    }

    if (path[1] == '\0') {
        return g_root;
    }

    const ramfs_node_t *current = g_root;
    const char *cursor = path + 1;

    while (*cursor != '\0') {
        const size_t len = segment_length(cursor);
        if (len == 0) {
            ++cursor;
            continue;
        }

        current = ramfs_find_child_const(current, cursor, len);
        if (current == 0) {
            return 0;
        }

        cursor += len;
        if (*cursor == '/') {
            ++cursor;
        }
    }

    return current;
}

static bool ramfs_split_parent(const char *path, char *name_out, size_t name_out_size, ramfs_node_t **parent_out) {
    if (g_root == 0 || path == 0 || path[0] != '/' || path[1] == '\0') {
        return false;
    }

    const char *last_slash = path;
    for (const char *cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '/') {
            last_slash = cursor;
        }
    }

    const char *name = last_slash + 1;
    const size_t name_len = strlen(name);
    if (name_len == 0 || name_len >= name_out_size) {
        return false;
    }

    if (last_slash == path) {
        *parent_out = g_root;
    } else {
        char parent_path[128];
        const size_t parent_len = (size_t)(last_slash - path);
        if (parent_len >= sizeof(parent_path)) {
            return false;
        }

        memcpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';

        *parent_out = (ramfs_node_t *)ramfs_resolve_path(parent_path);
        if (*parent_out == 0 || !(*parent_out)->is_dir) {
            return false;
        }
    }

    memcpy(name_out, name, name_len);
    name_out[name_len] = '\0';
    return true;
}

static bool ramfs_write_bytes_internal(ramfs_node_t *node, const void *data, size_t size) {
    char *buffer = (char *)kmalloc(size + 1);
    if (buffer == 0) {
        return false;
    }

    memcpy(buffer, data, size);
    buffer[size] = '\0';
    node->data = buffer;
    node->size = size;
    return true;
}

static uint32_t tar_parse_octal(const char *text, size_t size) {
    uint32_t value = 0;

    for (size_t i = 0; i < size; ++i) {
        if (text[i] == '\0' || text[i] == ' ') {
            break;
        }
        value = (value << 3) + (uint32_t)(text[i] - '0');
    }

    return value;
}

static bool tar_header_empty(const tar_header_t *header) {
    for (size_t i = 0; i < sizeof(*header); ++i) {
        if (((const uint8_t *)header)[i] != 0u) {
            return false;
        }
    }

    return true;
}

static void tar_build_path(const tar_header_t *header, char *path, size_t path_size) {
    size_t offset = 0;
    path[offset++] = '/';

    if (header->prefix[0] != '\0') {
        size_t prefix_len = strlen(header->prefix);
        if (offset + prefix_len < path_size) {
            memcpy(path + offset, header->prefix, prefix_len);
            offset += prefix_len;
            path[offset++] = '/';
        }
    }

    const size_t name_len = strlen(header->name);
    if (offset + name_len >= path_size) {
        path[0] = '/';
        path[1] = '\0';
        return;
    }

    memcpy(path + offset, header->name, name_len);
    offset += name_len;
    path[offset] = '\0';

    if (offset > 1 && path[offset - 1] == '/') {
        path[offset - 1] = '\0';
    }
}

static bool ramfs_ensure_directory(const char *path) {
    if (path == 0 || path[0] != '/') {
        return false;
    }

    if (path[1] == '\0') {
        return true;
    }

    char partial[128];
    size_t index = 0;

    partial[index++] = '/';
    for (size_t i = 1; path[i] != '\0'; ++i) {
        if (index + 1 >= sizeof(partial)) {
            return false;
        }

        partial[index++] = path[i];
        partial[index] = '\0';

        if (path[i] == '/') {
            partial[index - 1] = '\0';
            if (!ramfs_is_dir(partial) && !ramfs_mkdir(partial)) {
                return false;
            }
            partial[index - 1] = '/';
        }
    }

    if (!ramfs_is_dir(partial) && !ramfs_mkdir(partial)) {
        return false;
    }

    return true;
}

static bool ramfs_ensure_parent_directory(const char *path) {
    char parent[128];
    const size_t length = strlen(path);

    if (length >= sizeof(parent) || length < 2) {
        return false;
    }

    memcpy(parent, path, length + 1);

    for (size_t i = length; i > 0; --i) {
        if (parent[i] == '/') {
            if (i <= 1) {
                return true;
            }
            parent[i] = '\0';
            return ramfs_ensure_directory(parent);
        }
    }

    return false;
}

void ramfs_initialize(void) {
    g_root = ramfs_alloc_node("/", true);
}

void ramfs_load_initrd(const void *address, size_t size) {
    const uint8_t *cursor = (const uint8_t *)address;
    const uint8_t *end = cursor + size;

    while (cursor + sizeof(tar_header_t) <= end) {
        const tar_header_t *header = (const tar_header_t *)cursor;
        if (tar_header_empty(header)) {
            break;
        }

        char path[256];
        tar_build_path(header, path, sizeof(path));

        const uint32_t file_size = tar_parse_octal(header->size, sizeof(header->size));
        const uint8_t *data = cursor + 512u;

        if (header->typeflag == '5') {
            ramfs_ensure_directory(path);
        } else if (header->typeflag == '0' || header->typeflag == '\0') {
            ramfs_ensure_parent_directory(path);
            ramfs_touch(path);

            ramfs_node_t *node = (ramfs_node_t *)ramfs_resolve_path(path);
            if (node != 0 && !node->is_dir) {
                ramfs_write_bytes_internal(node, data, file_size);
            }
        }

        const uintptr_t next =
            ((uintptr_t)(cursor + 512u + file_size) + 511u) & ~(uintptr_t)511u;
        cursor = (const uint8_t *)next;
    }
}

bool ramfs_list(const char *path, ramfs_list_callback_t callback, void *context) {
    const ramfs_node_t *node = ramfs_resolve_path(path);
    if (node == 0 || !node->is_dir) {
        return false;
    }

    for (const ramfs_node_t *child = node->first_child; child != 0; child = child->next_sibling) {
        callback(child->name, child->is_dir, context);
    }
    return true;
}

bool ramfs_is_dir(const char *path) {
    const ramfs_node_t *node = ramfs_resolve_path(path);
    return node != 0 && node->is_dir;
}

bool ramfs_read(const char *path, const char **data, size_t *size) {
    const ramfs_node_t *node = ramfs_resolve_path(path);
    if (node == 0 || node->is_dir) {
        return false;
    }

    *data = node->data != 0 ? node->data : "";
    *size = node->size;
    return true;
}

bool ramfs_mkdir(const char *path) {
    char name[RAMFS_NODE_NAME_MAX + 1];
    ramfs_node_t *parent;

    if (!ramfs_split_parent(path, name, sizeof(name), &parent)) {
        return false;
    }

    if (ramfs_find_child(parent, name, strlen(name)) != 0) {
        return false;
    }

    return ramfs_add_child(parent, name, true) != 0;
}

bool ramfs_touch(const char *path) {
    char name[RAMFS_NODE_NAME_MAX + 1];
    ramfs_node_t *parent;

    if (!ramfs_split_parent(path, name, sizeof(name), &parent)) {
        return false;
    }

    if (ramfs_find_child(parent, name, strlen(name)) != 0) {
        return false;
    }

    return ramfs_add_child(parent, name, false) != 0;
}

bool ramfs_write(const char *path, const char *data) {
    ramfs_node_t *node = (ramfs_node_t *)ramfs_resolve_path(path);

    if (node == 0) {
        if (!ramfs_touch(path)) {
            return false;
        }
        node = (ramfs_node_t *)ramfs_resolve_path(path);
    }

    if (node == 0 || node->is_dir) {
        return false;
    }

    return ramfs_write_bytes_internal(node, data, strlen(data));
}
