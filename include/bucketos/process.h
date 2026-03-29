// process.h: Process/task structures, user register snapshots, and fd table types.

/*
 * Reading guide:
 * - Purpose: process.h: Process/task structures, user register snapshots, and fd table types.
 * - Start reading at: process_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_PROCESS_H
#define BUCKETOS_PROCESS_H

#include "bucketos/common.h"
#include "bucketos/idt.h"
#include "bucketos/paging.h"

typedef enum {
    PROCESS_STATE_UNUSED = 0,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_EXITED
} process_state_t;

typedef enum {
    PROCESS_FD_NONE = 0,
    PROCESS_FD_CONSOLE,
    PROCESS_FD_FILE,
    PROCESS_FD_NULL,
    PROCESS_FD_FRAMEBUFFER,
    PROCESS_FD_MOUSE
} process_fd_type_t;

typedef struct {
    bool in_use;
    process_fd_type_t type;
    const char *data;
    size_t size;
    size_t offset;
} process_fd_t;

enum {
    PROCESS_FD_MAX = 8
};

typedef struct {
    uint32_t pid;
    process_state_t state;
    registers_t regs;
    uintptr_t page_directory;
    uintptr_t image_base;
    size_t image_size;
    uintptr_t stack_bottom;
    uintptr_t stack_top;
    uintptr_t entry;
    uint32_t exit_code;
    char name[32];
    process_fd_t fds[PROCESS_FD_MAX];
} process_t;

void process_initialize(void);
process_t *process_current(void);
void process_set_current(process_t *process);
process_t *process_create(const char *name, uintptr_t entry, const user_space_mapping_t *mapping);
void process_mark_running(process_t *process);
void process_mark_exited(process_t *process, uint32_t exit_code);
void process_reset_fds(process_t *process);
bool process_validate_user_range(const process_t *process, uintptr_t address, size_t size);
bool process_copy_from_user(void *dst, const void *src, size_t size);
bool process_copy_to_user(void *dst, const void *src, size_t size);
bool process_copy_user_string(char *dst, size_t dst_size, const char *src);

#endif
