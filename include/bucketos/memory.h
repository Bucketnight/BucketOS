#ifndef BUCKETOS_MEMORY_H
#define BUCKETOS_MEMORY_H

#include "bucketos/common.h"
#include "bucketos/multiboot.h"

#define MEMORY_REGION_MAX 32

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} memory_region_t;

typedef struct {
    uint32_t mem_lower_kib;
    uint32_t mem_upper_kib;
    uint64_t usable_bytes;
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t heap_current;
    memory_region_t regions[MEMORY_REGION_MAX];
    size_t region_count;
} memory_info_t;

void memory_initialize(const multiboot_info_t *multiboot);
const memory_info_t *memory_info(void);
void *kmalloc(size_t size);
void *kmalloc_aligned(size_t size, size_t alignment);

#endif
