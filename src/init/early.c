#include <stdbool.h>
#include <x86/gdt.h>
#include <x86/idt.h>
#include <x86/log.h>
#include <stdio.h>
#include <acpi/acpi.h>
#include <mem/gheap.h>
#include <mem/physical.h>
#include <mem/virtual.h>
#include <vfs/vfs.h>
#include <sched/sched.h>
#include <x86/apic.h>

void kernel_start() {
    gdt_init();
    idt_init();
    pmem_init();
    vmem_init();
    gheap_init();
    tss_init();
    vfs_init();
    acpi_init();
    apic_init();
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

    // enable interrupts
    asm volatile ("sti");

    while (true) {
        asm volatile ("nop");
    }
}
