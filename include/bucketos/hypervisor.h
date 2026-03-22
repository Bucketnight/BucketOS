#ifndef BUCKETKERNEL_HYPERVISOR_H
#define BUCKETKERNEL_HYPERVISOR_H

#include "bucketos/common.h"

typedef struct {
    bool present;
    char vendor[13];
} hypervisor_info_t;

hypervisor_info_t hypervisor_detect(void);

#endif
