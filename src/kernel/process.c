// process.c: Process implementation (fd table, user pointer copy helpers, state).

/*
 * Reading guide:
 * - Purpose: process.c: Process implementation (fd table, user pointer copy helpers, state).
 * - Start reading at: process_reset_fds
 * - Tip: Anything reachable from interrupts must stay simple (no blocking; be careful with shared state).
 */

#include "bucketos/process.h"
#include "bucketos/string.h"

enum {
    PROCESS_MAX = 4
};

static process_t g_processes[PROCESS_MAX];
static process_t *g_current_process;
static uint32_t g_next_pid;

static size_t process_string_length(const char *text, size_t limit) {
    size_t length = 0;
    while (length < limit && text[length] != '\0') {
        ++length;
    }
    return length;
}

void process_reset_fds(process_t *process) {
    if (process == 0) {
        return;
    }

    memset(process->fds, 0, sizeof(process->fds));
    process->fds[0].in_use = true;
    process->fds[0].type = PROCESS_FD_CONSOLE;
    process->fds[1].in_use = true;
    process->fds[1].type = PROCESS_FD_CONSOLE;
    process->fds[2].in_use = true;
    process->fds[2].type = PROCESS_FD_CONSOLE;
}

void process_initialize(void) {
    memset(g_processes, 0, sizeof(g_processes));
    g_current_process = 0;
    g_next_pid = 1;
}

process_t *process_current(void) {
    return g_current_process;
}

void process_set_current(process_t *process) {
    g_current_process = process;
}

process_t *process_create(const char *name, uintptr_t entry, const user_space_mapping_t *mapping) {
    process_t *process = 0;

    for (size_t index = 0; index < PROCESS_MAX; ++index) {
        if (g_processes[index].state == PROCESS_STATE_UNUSED
            || g_processes[index].state == PROCESS_STATE_EXITED) {
            process = &g_processes[index];
            break;
        }
    }

    if (process == 0) {
        return 0;
    }

    memset(process, 0, sizeof(*process));
    process->pid = g_next_pid++;
    process->state = PROCESS_STATE_READY;
    process->image_base = mapping->image_base_virtual;
    process->image_size = mapping->image_size;
    process->stack_bottom = mapping->stack_bottom_virtual;
    process->stack_top = mapping->stack_top_virtual;
    process->entry = entry;
    process->regs.eip = (uint32_t)entry;
    process->regs.esp = (uint32_t)mapping->stack_top_virtual;
    process->regs.cs = 0x1Bu;
    process->regs.ds = 0x23u;
    process->regs.es = 0x23u;
    process->regs.fs = 0x23u;
    process->regs.gs = 0x23u;
    process->regs.eflags = 0x202u;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(process->page_directory));
    process_reset_fds(process);

    if (name != 0) {
        const size_t length = process_string_length(name, sizeof(process->name) - 1u);
        memcpy(process->name, name, length);
        process->name[length] = '\0';
    }

    return process;
}

void process_mark_running(process_t *process) {
    if (process != 0) {
        process->state = PROCESS_STATE_RUNNING;
        g_current_process = process;
    }
}

void process_mark_exited(process_t *process, uint32_t exit_code) {
    if (process != 0) {
        process->state = PROCESS_STATE_EXITED;
        process->exit_code = exit_code;
    }
}

bool process_validate_user_range(const process_t *process, uintptr_t address, size_t size) {
    uintptr_t end;

    if (process == 0) {
        return false;
    }

    if (__builtin_add_overflow(address, size, &end)) {
        return false;
    }

    if (address >= process->image_base && end <= process->image_base + process->image_size) {
        return true;
    }

    if (address >= process->stack_bottom && end <= process->stack_top) {
        return true;
    }

    return false;
}

bool process_copy_from_user(void *dst, const void *src, size_t size) {
    const process_t *const process = process_current();

    if (!process_validate_user_range(process, (uintptr_t)src, size)) {
        return false;
    }

    memcpy(dst, src, size);
    return true;
}

bool process_copy_to_user(void *dst, const void *src, size_t size) {
    const process_t *const process = process_current();

    if (!process_validate_user_range(process, (uintptr_t)dst, size)) {
        return false;
    }

    memcpy(dst, src, size);
    return true;
}

bool process_copy_user_string(char *dst, size_t dst_size, const char *src) {
    const process_t *const process = process_current();
    size_t length = 0;

    if (process == 0 || dst == 0 || dst_size == 0 || src == 0) {
        return false;
    }

    while (length + 1 < dst_size) {
        if (!process_validate_user_range(process, (uintptr_t)(src + length), 1u)) {
            return false;
        }

        dst[length] = src[length];
        if (dst[length] == '\0') {
            return true;
        }

        ++length;
    }

    dst[length] = '\0';
    return false;
}
