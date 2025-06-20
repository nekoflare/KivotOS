#include <stdbool.h>
#include <x86/gdt.h>
#include <x86/idt.h>
#include <x86/log.h>
#include <stdio.h>
#include <string.h>
#include <acpi/acpi.h>
#include <mem/gheap.h>
#include <mem/physical.h>
#include <mem/virtual.h>
#include <mem/dlmalloc.h>
#include <vfs/vfs.h>
#include <elf/elf.h>
#include <sched/sched.h>
#include <x86/apic.h>
#include <sched/tls.h>

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

    create_user_process("/init", NULL, NULL);

    // enable interrupts
    asm volatile ("sti");

    while (true) {
        asm volatile ("nop");
    }
}
