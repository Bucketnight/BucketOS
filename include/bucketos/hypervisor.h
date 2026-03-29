// hypervisor.h: Hypervisor detection API (CPUID hypervisor leaf).

/*
 * Reading guide:
 * - Purpose: hypervisor.h: Hypervisor detection API (CPUID hypervisor leaf).
 * - Start reading at: hypervisor_detect
 * - Tip: This is part of the public API; keep it stable and document any assumptions.
 */

#ifndef BUCKETOS_HYPERVISOR_H
#define BUCKETOS_HYPERVISOR_H

#include "bucketos/common.h"

typedef struct {
    bool present;
    char vendor[13];
    const char *name; 
} hypervisor_info_t;

hypervisor_info_t hypervisor_detect(void);

#endif
