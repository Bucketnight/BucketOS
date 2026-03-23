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
