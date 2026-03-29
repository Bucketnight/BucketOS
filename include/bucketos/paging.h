// paging.h: Paging API and user-space mapping description.

/*
 * Reading guide:
 * - Purpose: paging.h: Paging API and user-space mapping description.
 * - Start reading at: paging_initialize
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_PAGING_H
#define BUCKETOS_PAGING_H

#include "bucketos/common.h"
#include "bucketos/framebuffer.h"

typedef struct {
    uintptr_t image_base_virtual;
    size_t image_size;
    uintptr_t stack_bottom_virtual;
    uintptr_t stack_top_virtual;
} user_space_mapping_t;

void paging_initialize(const framebuffer_info_t *framebuffer);
bool paging_is_enabled(void);
const user_space_mapping_t *paging_user_space(void);
void paging_map_initial_user_space(void);

#endif
