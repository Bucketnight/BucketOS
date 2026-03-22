/* cpuid.h: detects hpervisor or real hardware */
#ifndef BUCKETKERNEL_CPUID_H
#define BUCKETKERNEL_CPUID_H

#include "bucketos/common.h"

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} cpuid_result_t;

static inline cpuid_result_t cpuid(uint32_t leaf) {
    cpuid_result_t result;

    __asm__ volatile (
        "cpuid"
        : "=a"(result.eax), "=b"(result.ebx), "=c"(result.ecx), "=d"(result.edx)
        : "a"(leaf)
    );

    return result;
}

#endif
