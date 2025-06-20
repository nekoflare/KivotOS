//
// Created by neko on 6/11/25.
//

#ifndef TLS_H
#define TLS_H

#define MSR_IA32_CURRENT_GS_BASE 0xC0000101
#define MSR_IA32_KERNEL_GS_BASE 0xC0000102

struct kernel_tls_data {
    struct task* current_process;    // the pointer to the current process.
};

void set_current_tls(struct kernel_tls_data* data);
void set_kernel_tls(struct kernel_tls_data* data);
struct kernel_tls_data* get_current_tls();
struct kernel_tls_data* get_kernel_tls();

#endif //TLS_H
