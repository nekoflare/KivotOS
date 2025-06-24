//
// Created by neko on 6/21/25.
//

#ifndef SYSCALL_SYSTEM_H
#define SYSCALL_SYSTEM_H

#include <stdint.h>

#define IA32_EFER                0xC0000080
#define IA32_STAR               0xC0000081
#define IA32_LSTAR              0xC0000082
#define IA32_FMASK              0xC0000084

#define KERNEL_CS 0x8
#define USER_CS 0x18 // it will work.

struct syscall_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rbp, rdx, rcx, rbx, rax, pad;
};

extern void syscall_handler();

void syscall_dispatch(struct syscall_frame *frame);
void syscall_system_init();

#endif //SYSCALL_SYSTEM_H
