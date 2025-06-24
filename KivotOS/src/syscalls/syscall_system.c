//
// Created by neko on 6/21/25.
//qq

#include "syscall_system.h"
#include "syscalls.h"

#include <stdlib.h>
#include <sched/sched.h>
#include <x86/log.h>
#include <x86/msr.h>

uint64_t kernel_rsp = 0;
uint64_t user_rsp = 0;

#define kvt_write   (1)
#define kvt_exit    (100)
#define kvt_set_tcb (200)

void syscall_dispatch(struct syscall_frame *frame) {
    switch (frame->rax) {
        case kvt_write: // sys_write
        {
            frame->rax = syscall_write(
                (int) (frame->rdi),
                (const void *) (frame->rsi),
                (size_t) (frame->rdx));
            break;
        }
        case kvt_set_tcb: // kvt_set_tcb
        {
            // finna set that fs base twin
            debug_print("Setting process %d TCB to -> %p\n", get_current_process()->tid, (void*)(frame->rdi));
            wrmsr(0xc0000100, frame->rdi);
            break;
        }
        default: {
            debug_print("Unknown syscall %lu\n", frame->rax);
            asm volatile ("cli; hlt");
        }
    }
}

void syscall_system_init() {
    kernel_rsp = malloc(4 * 1024 * 1024);
    kernel_rsp &= ~0xFULL; // alignment
    uint64_t lstar = (uint64_t)syscall_handler;
    wrmsr(IA32_EFER, rdmsr(IA32_EFER) | (1 << 0)); // SCE bit
    wrmsr(IA32_LSTAR, lstar);
    wrmsr(IA32_FMASK, 1 << 9); // Mask IF bits on syscall
    wrmsr(IA32_STAR, ((uint64_t)(KERNEL_CS) << 32) | ((uint64_t)(USER_CS) << 48));

    uint64_t star = rdmsr(0xC0000081);
    debug_print("IA32_STAR = 0x%lx\n", star);
}
