#include "bucketos/memory.h"
#include "bucketos/string.h"

extern char kernel_end;

static memory_info_t g_memory_info;

static uintptr_t align_up(uintptr_t value, uintptr_t alignment) {
    const uintptr_t mask = alignment - 1;
    return (value + mask) & ~mask;
}

void memory_initialize(const multiboot_info_t *multiboot) {
    memset(&g_memory_info, 0, sizeof(g_memory_info));

    g_memory_info.mem_lower_kib = multiboot->mem_lower;
    g_memory_info.mem_upper_kib = multiboot->mem_upper;

    if ((multiboot->flags & MULTIBOOT_INFO_MEM_MAP) != 0u) {
        uintptr_t current = (uintptr_t)multiboot->mmap_addr;
        const uintptr_t end = current + multiboot->mmap_length;

        while (current < end) {
            const multiboot_mmap_entry_t *entry = (const multiboot_mmap_entry_t *)current;

            if (g_memory_info.region_count < MEMORY_REGION_MAX) {
                memory_region_t*region = &g_memory_info.regions[g_memory_info.region_count++];
                region->base = entry->addr;
                region->length = entry->len;
                region-> type = entry->type;
            }

            if (entry->type == 1u) {
                g_memory_info.usable_bytes += entry->len;
            }

            current += entry->size + sizeof(entry->size);
        }
    } else {
        g_memory_info.usable_bytes =
            ((uint64_t)multiboot->mem_lower + (uint64_t)multiboot->mem_upper) * 1024u;
    }

    g_memory_info.heap_start = align_up((uintptr_t)&kernel_end, 4096u);
    g_memory_info.heap_end = g_memory_info.heap_start + (1024u * 1024u);
    g_memory_info.heap_current = g_memory_info.heap_start;
}

const memory_info_t *memory_info(void) {
    return &g_memory_info;
}

void *kmalloc(size_t size) {
    const uintptr_t start = align_up(g_memory_info.heap_current, 16u);
    const uintptr_t end = start + size;

    if (end > g_memory_info.heap_end) {
        return 0;
    }

    g_memory_info.heap_current = end;
    return (void *)start;
}
