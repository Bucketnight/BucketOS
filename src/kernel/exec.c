#include "bucketos/elf.h"
#include "bucketos/exec.h"
#include "bucketos/gdt.h"
#include "bucketos/paging.h"
#include "bucketos/process.h"
#include "bucketos/scheduler.h"
#include "bucketos/string.h"
#include "bucketos/syscall.h"
#include "bucketos/vfs.h"

enum {
    EXEC_ARG_MAX = 16
};

static uintptr_t g_exec_entry;
static process_t *g_exec_process;

static bool elf_header_valid(const elf32_header_t *header, size_t size) {
    return size >= sizeof(*header)
        && *(const uint32_t *)header->ident == ELF_MAGIC
        && header->ident[4] == ELFCLASS32
        && header->ident[5] == ELFDATA2LSB
        && header->type == ET_EXEC
        && header->machine == EM_386
        && header->phentsize == sizeof(elf32_program_header_t);
}

static bool exec_region_contains(const user_space_mapping_t *mapping, uintptr_t start, uintptr_t size) {
    const uintptr_t region_end = mapping->image_base_virtual + mapping->image_size;
    const uintptr_t end = start + size;

    return start >= mapping->image_base_virtual && end >= start && end <= region_end;
}

bool exec_load(const char *path) {
    const char *data;
    size_t size;
    const user_space_mapping_t *const mapping = paging_user_space();

    if (!vfs_read(path, &data, &size)) {
        return false;
    }

    const elf32_header_t *const header = (const elf32_header_t *)data;
    if (!elf_header_valid(header, size)) {
        return false;
    }

    if ((size_t)header->phoff + (size_t)header->phnum * sizeof(elf32_program_header_t) > size) {
        return false;
    }

    memset((void *)mapping->image_base_virtual, 0, mapping->image_size);

    const elf32_program_header_t *const programs =
        (const elf32_program_header_t *)(data + header->phoff);

    for (uint16_t index = 0; index < header->phnum; ++index) {
        const elf32_program_header_t *const program = &programs[index];

        if (program->type != PT_LOAD) {
            continue;
        }

        if ((size_t)program->offset + program->filesz > size) {
            return false;
        }

        if (!exec_region_contains(mapping, program->vaddr, program->memsz)) {
            return false;
        }

        memset((void *)program->vaddr, 0, program->memsz);
        memcpy((void *)program->vaddr, data + program->offset, program->filesz);
    }

    if (!exec_region_contains(mapping, header->entry, 1u)) {
        return false;
    }

    g_exec_entry = header->entry;
    g_exec_process = process_create(path, header->entry, mapping);
    if (g_exec_process == 0) {
        return false;
    }

    scheduler_register(g_exec_process);
    return true;
}

static uintptr_t exec_prepare_stack(
    const user_space_mapping_t *mapping, int argc, const char *const argv[]) {
    uintptr_t stack_pointer = mapping->stack_top_virtual;
    uintptr_t user_argv[EXEC_ARG_MAX];

    if (argc < 0) {
        argc = 0;
    }
    if (argc > EXEC_ARG_MAX) {
        argc = EXEC_ARG_MAX;
    }

    for (int index = argc - 1; index >= 0; --index) {
        const size_t length = strlen(argv[index]) + 1u;
        stack_pointer -= length;
        memcpy((void *)stack_pointer, argv[index], length);
        user_argv[index] = stack_pointer;
    }

    stack_pointer &= ~(uintptr_t)0x3u;
    stack_pointer -= sizeof(uintptr_t);
    *(uintptr_t *)stack_pointer = 0;

    for (int index = argc - 1; index >= 0; --index) {
        stack_pointer -= sizeof(uintptr_t);
        *(uintptr_t *)stack_pointer = user_argv[index];
    }

    {
        const uintptr_t argv_user = stack_pointer;
        stack_pointer -= sizeof(uintptr_t);
        *(uintptr_t *)stack_pointer = argv_user;
    }

    stack_pointer -= sizeof(uint32_t);
    *(uint32_t *)stack_pointer = (uint32_t)argc;
    return stack_pointer;
}

uint32_t exec_run_argv(const char *path, int argc, const char *const argv[]) {
    const user_space_mapping_t *const mapping = paging_user_space();
    const uintptr_t user_stack =
        exec_prepare_stack(mapping, argc, argv != 0 ? argv : (const char *const *)0);

    if (!exec_load(path)) {
        return 0xFFFFFFFFu;
    }

    scheduler_set_current(g_exec_process);
    process_mark_running(g_exec_process);
    syscall_reset_process();
    return gdt_enter_user_mode(g_exec_entry, user_stack);
}

uint32_t exec_run(const char *path) {
    return exec_run_argv(path, 0, 0);
}
