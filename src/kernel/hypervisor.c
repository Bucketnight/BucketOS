#include "bucketkernel/hypervisor.h"
#include "bucketkernel/cpuid.h"

hypervisor_info_t hypervisor_detect(void) {
    hypervisor_info_t info;
    info.present = false;
    info.vendor[0] = '\0';

    cpuid_result_t leaf1 = cpuid(0x1);

    if ((leaf1.ecx & (1u << 31)) == 0) {
        return info;
    }

    cpuid_result_t hv = cpuid(0x40000000);
    info.present = true;

    ((uint32_t *)info.vendor)[0] = hv.ebx;
    ((uint32_t *)info.vendor)[1] = hv.ecx;
    ((uint32_t *)info.vendor)[2] = hv.edx;
    info.vendor[12] = '\0';

    return info;
}
