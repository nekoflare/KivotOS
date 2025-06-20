//
// Created by neko on 6/11/25.
//

#include "tls.h"

#include <x86/msr.h>

void set_current_tls(struct kernel_tls_data *data) {
    wrmsr(MSR_IA32_CURRENT_GS_BASE, (uint64_t)data);
}

void set_kernel_tls(struct kernel_tls_data *data) {
    wrmsr(MSR_IA32_KERNEL_GS_BASE, (uint64_t)data);
}

struct kernel_tls_data * get_kernel_tls() {
    return (struct kernel_tls_data *)rdmsr(MSR_IA32_KERNEL_GS_BASE);
}

struct kernel_tls_data* get_current_tls() {
    struct kernel_tls_data* tls;
    __asm__ volatile ("mov %%gs:0, %0" : "=r"(tls));
    return tls;
}
