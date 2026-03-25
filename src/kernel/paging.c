#include "bucketos/paging.h"
#include "bucketos/memory.h"
#include "bucketos/panic.h"
#include "bucketos/string.h"

enum {
    PAGE_SIZE = 4096u,
    PAGE_DIRECTORY_ENTRIES = 1024u,
    PAGE_TABLE_ENTRIES = 1024u,
    PAGE_TABLE_POOL_COUNT = 64u,
    PAGE_PRESENT = 0x001u,
    PAGE_WRITABLE = 0x002u,
    PAGE_USER = 0x004u,
    MIN_IDENTITY_MAP_BYTES = 16u * 1024u * 1024u,
    USER_CODE_VIRTUAL_BASE = 0x00400000u,
    USER_STACK_TOP = 0x00800000u
};

static uint32_t g_page_directory[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static uint32_t g_page_tables[PAGE_TABLE_POOL_COUNT][PAGE_TABLE_ENTRIES]
    __attribute__((aligned(PAGE_SIZE)));
static size_t g_page_table_count;
static bool g_paging_enabled;
static user_space_mapping_t g_user_space;

static uintptr_t align_down(uintptr_t value, uintptr_t alignment) {
    return value & ~(alignment - 1u);
}

static uintptr_t align_up(uintptr_t value, uintptr_t alignment) {
    return (value + alignment - 1u) & ~(alignment - 1u);
}

static uint32_t *paging_allocate_table(void) {
    if (g_page_table_count >= PAGE_TABLE_POOL_COUNT) {
        panic("paging: page table pool exhausted");
    }

    uint32_t *const table = g_page_tables[g_page_table_count++];
    memset(table, 0, PAGE_SIZE);
    return table;
}

static uint32_t *paging_table_for(uintptr_t virtual_address, bool create) {
    const uint32_t directory_index = (uint32_t)(virtual_address >> 22);

    if ((g_page_directory[directory_index] & PAGE_PRESENT) == 0u) {
        if (!create) {
            return 0;
        }

        uint32_t *const table = paging_allocate_table();
        g_page_directory[directory_index] =
            ((uintptr_t)table & 0xFFFFF000u) | PAGE_PRESENT | PAGE_WRITABLE;
        return table;
    }

    return (uint32_t *)(uintptr_t)(g_page_directory[directory_index] & 0xFFFFF000u);
}

static void paging_map_page(uintptr_t virtual_address, uintptr_t physical_address, uint32_t flags) {
    uint32_t *const table = paging_table_for(virtual_address, true);
    const uint32_t table_index = (uint32_t)((virtual_address >> 12) & 0x3FFu);

    table[table_index] = (uint32_t)(physical_address & 0xFFFFF000u) | flags | PAGE_PRESENT;
}

static void paging_identity_map_range(uintptr_t start, uintptr_t end, uint32_t flags) {
    uintptr_t address = align_down(start, PAGE_SIZE);
    const uintptr_t limit = align_up(end, PAGE_SIZE);

    while (address < limit) {
        paging_map_page(address, address, flags);
        address += PAGE_SIZE;
    }
}

void paging_initialize(const framebuffer_info_t *framebuffer) {
    const memory_info_t *const info = memory_info();
    uintptr_t low_identity_end = info->heap_end;

    if (low_identity_end < MIN_IDENTITY_MAP_BYTES) {
        low_identity_end = MIN_IDENTITY_MAP_BYTES;
    }

    memset(g_page_directory, 0, sizeof(g_page_directory));
    g_page_table_count = 0;

    paging_identity_map_range(0u, low_identity_end, PAGE_WRITABLE);

    if (framebuffer != 0 && framebuffer->available && framebuffer->address != 0u) {
        const uintptr_t framebuffer_start = align_down(framebuffer->address, PAGE_SIZE);
        const uintptr_t framebuffer_end =
            align_up(framebuffer->address + (uintptr_t)framebuffer->pitch * framebuffer->height,
                PAGE_SIZE);
        paging_identity_map_range(framebuffer_start, framebuffer_end, PAGE_WRITABLE);
    }

    __asm__ volatile ("mov %0, %%cr3" : : "r"(g_page_directory) : "memory");

    uintptr_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");

    g_paging_enabled = true;
}

bool paging_is_enabled(void) {
    return g_paging_enabled;
}

const user_space_mapping_t *paging_user_space(void) {
    return &g_user_space;
}

void paging_map_initial_user_space(void) {
    void *const code_page = kmalloc(PAGE_SIZE);
    void *const stack_page = kmalloc(PAGE_SIZE);

    if (code_page == 0 || stack_page == 0) {
        panic("paging: unable to allocate initial user pages");
    }

    memset(code_page, 0, PAGE_SIZE);
    memset(stack_page, 0, PAGE_SIZE);

    g_user_space.code_virtual = USER_CODE_VIRTUAL_BASE;
    g_user_space.code_physical = (uintptr_t)code_page;
    g_user_space.stack_bottom_virtual = USER_STACK_TOP - PAGE_SIZE;
    g_user_space.stack_top_virtual = USER_STACK_TOP;
    g_user_space.stack_physical = (uintptr_t)stack_page;

    paging_map_page(g_user_space.code_virtual, g_user_space.code_physical, PAGE_WRITABLE | PAGE_USER);
    paging_map_page(
        g_user_space.stack_bottom_virtual, g_user_space.stack_physical, PAGE_WRITABLE | PAGE_USER);

    __asm__ volatile ("mov %0, %%cr3" : : "r"(g_page_directory) : "memory");
}
