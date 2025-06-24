//
// Created by neko on 6/11/25.
//

#ifndef TLS_H
#define TLS_H

#include <stdint.h>

#define MSR_IA32_CURRENT_GS_BASE 0xC0000101
#define MSR_IA32_KERNEL_GS_BASE 0xC0000102

struct kernel_tls_data {
    struct task* current_process;    // the pointer to the current process.
    uint64_t rsp3_stack; // for syscalls onlyyy
    uint64_t rsp0_stack; // for syscalls onlyyy too.
};

void set_tls(struct kernel_tls_data* data);
void set_kernel_tls(struct kernel_tls_data* data);
struct kernel_tls_data* get_tls();

#endif //TLS_H
