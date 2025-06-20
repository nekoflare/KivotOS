#include "gdt.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"

// null, kern code, kern data, user code, user data, tsss..... :snake:
struct gdt_entry gdt_entries[7] = {};
struct tss tss;
struct gdtr gdtr;

struct gdt_entry create_gdt_entry(uint32_t base, uint32_t limit, uint8_t flags, uint8_t access) {
    struct gdt_entry entry;

    entry.limit0 = limit & 0xFFFF;
    entry.limit1 = (limit >> 16) & 0xF;
    entry.base0  = base & 0xFFFF;
    entry.base1  = (base >> 16) & 0xFF;
    entry.base2  = (base >> 24) & 0xFF;
    entry.access = access;
    entry.flags  = flags & 0xF;

    return entry;
}

void gdt_init() {
    // NULL descriptor
    gdt_entries[0] = create_gdt_entry(0, 0, 0, 0);

    // Kernel code segment
    gdt_entries[1] = create_gdt_entry(0, 0xFFFFF,
        GDT_FLAG_LM | GDT_FLAG_G,
        GDT_ACC_P | GDT_ACC_S | GDT_ACC_X | GDT_ACC_RW | GDT_ACC_DPL(0));

    // Kernel data segment
    gdt_entries[2] = create_gdt_entry(0, 0xFFFFF,
        GDT_FLAG_G,
        GDT_ACC_P | GDT_ACC_S | GDT_ACC_RW | GDT_ACC_DPL(0));

    // User code segment
    gdt_entries[3] = create_gdt_entry(0, 0xFFFFF,
        GDT_FLAG_LM | GDT_FLAG_G,
        GDT_ACC_P | GDT_ACC_S | GDT_ACC_X | GDT_ACC_RW | GDT_ACC_DPL(3));

    // User data segment
    gdt_entries[4] = create_gdt_entry(0, 0xFFFFF,
        GDT_FLAG_G,
        GDT_ACC_P | GDT_ACC_S | GDT_ACC_RW | GDT_ACC_DPL(3));

    gdtr.limit = sizeof(gdt_entries) - 1;
    gdtr.base  = (uint64_t)&gdt_entries;

    flush_gdt(0x08, 0x10); // 0x08 = kern code, 0x10 = kern data
}

void create_tss_descriptor(struct gdt_entry* entry, uintptr_t base, uint32_t limit) {
    memset(entry, 0, sizeof(struct gdt_entry) * 2);

    entry[0].limit0 = limit & 0xFFFF;
    entry[0].limit1 = (limit >> 16) & 0xF;
    entry[0].base0  = base & 0xFFFF;
    entry[0].base1  = (base >> 16) & 0xFF;
    entry[0].base2  = (base >> 24) & 0xFF;

    // Present (P=1), Descriptor type (S=0 for system), Type=0x9 (available 64-bit TSS)
    entry[0].access = 0x89;

    // granularity=0, 64-bit=0, reserved=0
    entry[0].flags = 0x0;

    uint32_t* second_entry = (uint32_t*)&entry[1];
    second_entry[0] = (uint32_t)(base >> 32);
    second_entry[1] = 0;  // Reserved
}

extern void load_tss();


void tss_init() {
    memset(&tss, 0, sizeof(tss));

    // Allocate ring 0 stack for TSS (rsp0)
    void* ring0_stack = malloc(1 * 1024 * 1024 * 1024ULL); // 64KB stack
    if (!ring0_stack) {
        debug_print("Failed to allocate TSS stack\n");
    }

    tss.rsp0 = (uint64_t)ring0_stack + (1 * 1024 * 1024ULL); // rsp0 points to top of stack

    // align the rsp0.
    tss.rsp0 &= ~0xFULL;

    // Create TSS descriptor at GDT entry 5 (selector 0x28)
    create_tss_descriptor(&gdt_entries[5], (uintptr_t)&tss, sizeof(tss) - 1);

    // Reload GDT (if needed), then load TSS
    // Assuming gdt is already loaded and selectors set correctly
    debug_print("Loading TSS at %p\n", &tss);
    load_tss();
}