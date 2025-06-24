#include <stdbool.h>
#include <x86/gdt.h>
#include <x86/idt.h>
#include <x86/log.h>
#include <stdio.h>
#include <acpi/acpi.h>
#include <mem/gheap.h>
#include <mem/physical.h>
#include <mem/virtual.h>
#include <rt/instrumenting.h>
#include <vfs/vfs.h>
#include <sched/sched.h>
#include <syscalls/syscall_system.h>
#include <x86/apic.h>
#include <sched/tls.h>
#include <x86/interrupts_helpers.h>

void create_tls() {
    // creates tls..
    struct kernel_tls_data* tls = malloc(sizeof(struct kernel_tls_data));

    tls->current_process = NULL; // unused..
    tls->rsp3_stack = 0; // no stack.
    tls->rsp0_stack = malloc(2 * 1024 * 1024); // 1MB of stack only for syscall...

    tls->rsp0_stack &= ~(0xFULL);

    uint64_t* things = tls;
    for (int i = 0; 3 > i; i++) {
        debug_print("%016lX ", things[i]);
    }

    set_tls(tls);
    set_kernel_tls(tls);
}

void kernel_start() {
    gdt_init();
    idt_init();
    physical_memory_init();
    vmem_init();
    gheap_init();
    tss_init();
    vfs_init();
    acpi_init();
    apic_init();
    create_tls();
    syscall_system_init();
    sched_init();
    acpi_further_init();

    debug_log("Architecture initialized\n");

    struct task* current_process = get_current_process();
    int current_tid = current_process->tid;
    int current_priority = current_process->priority;

    debug_print("Current TID: %d Current priority: %d\n", current_tid, current_priority);

    create_user_process("/init", NULL, NULL, "/init", current_tid, current_priority);

    // time to print processes :3
    print_processes();

    flush_gdt(0x8, 0x10); // flush gdt caches cuh

    // check if interrupts were already enabled. shouldnt be enabled though.
    unsigned long cpu_flags = get_cpu_flags();
    if (cpu_flags & (1 << 9)) {
        debug_log("Interrupts were already enabled. This is bad.\n");
        asm volatile ("cli; hlt");
    }

    // enable interrupts
    asm volatile ("sti");

    // we're live!

    while (true) {
        asm volatile ("nop");
    }
}
