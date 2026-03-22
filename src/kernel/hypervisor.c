#include "bucketos/hypervisor.h"
#include "bucketos/cpuid.h"
#include "bucketos/string.h"

static const char*hypervisor_name_from_vendor(const char *vendor) {
    if (strcmp(vendor, "TCGTCGTCGTCG") == 0) {
        return "QEMU";
    }
    if (strcmp(vendor, "KVMKVMKVM") == 0) {
        return "KVM";
    }
    if (strcmp(vendor, "Microsoft Hv") == 0) {
        return "HyperV";
    }
    if (strcmp(vendor, "VMwareVMware") == 0) {
        return "VMWare"; 
    }
    if (strcmp(vendor, "XenVMMXenVMM") == 0) {
        return "Xen";
    }
    if (strcmp(vendor, "VBoxVBoxVBox") == 0) {
        return "VirtualBox"; /* VirtualBox is not exposing the hypervisor-present bit to the guest. So bare metal is displayed. */
    }
    if (strcmp(vendor, "prl hyperv") == 0) {
        return "Parallels";
    }
    return "Unknown";
}

hypervisor_info_t hypervisor_detect(void) {
    hypervisor_info_t info;
    info.present = false;
    info.vendor[0] = '\0';
    info.name = "Bare metal";

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

    info.name = hypervisor_name_from_vendor(info.vendor);
    return info;
}
