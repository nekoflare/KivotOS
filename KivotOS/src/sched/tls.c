//
// Created by neko on 6/11/25.
//

#include "tls.h"

#include <x86/msr.h>

void set_tls(struct kernel_tls_data *data) {
    wrmsr(MSR_IA32_CURRENT_GS_BASE, (uint64_t)data);
}

void set_kernel_tls(struct kernel_tls_data *data) {
    wrmsr(MSR_IA32_KERNEL_GS_BASE, (uint64_t)data);
}

struct kernel_tls_data * get_tls() {
    return (struct kernel_tls_data *) rdmsr(MSR_IA32_CURRENT_GS_BASE);
}
