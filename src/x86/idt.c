//
// Created by neko on 6/7/25.
//

#include "idt.h"

#include <stddef.h>
#include <stdlib.h>

#include "apic.h"
#include "log.h"

struct idtr idtr;
struct idt_entry idt_entries[256] = {};
extern uint64_t exception_handler_pointers[256];

struct interrupt_redirection_entry* int_redir_table = NULL;

struct idt_entry create_idt_entry(uint64_t offset, uint16_t segment_selector, uint8_t dpl, bool present) {
    struct idt_entry entry;

    entry.offset0 = offset & 0xFFFF;
    entry.segment_selector = segment_selector;

    entry.ist = 0; // no IST
    // Type attribute byte:
    // bit 7 = present
    // bits 6-5 = DPL (Descriptor Privilege Level)
    // bit 4 = storage segment (0 for interrupt/trap gates)
    // bits 3-0 = gate type (0xE = 32-bit interrupt gate)
    entry.type_attr = (present << 7) | (dpl << 5) | (0 << 4) | 0xE;

    entry.offset1 = (offset >> 16) & 0xFFFF;
    entry.offset2 = (offset >> 32) & 0xFFFFFFFF;
    entry.zero = 0;

    return entry;
}

void print_page_fault_error_code(uint64_t error_code) {
    debug_print("Page Fault Error Code: 0x%lx\n", error_code);

    bool present = error_code & 0x1;
    bool write = error_code & 0x2;
    bool user = error_code & 0x4;
    bool reserved = error_code & 0x8;
    bool instr_fetch = error_code & 0x10;

    debug_print(" - Page %s\n", present ? "present (protection violation)" : "not present");
    debug_print(" - Access Type: %s\n", write ? "Write" : "Read");
    debug_print(" - Mode: %s mode\n", user ? "User" : "Supervisor");
    if (reserved) debug_print(" - Reserved bits violation\n");
    if (instr_fetch) debug_print(" - Instruction fetch\n");
}


void uint64_to_binary_str(uint64_t value, char *buf) {
    for (int i = 63; i >= 0; i--) {
        buf[63 - i] = (value & ((uint64_t)1 << i)) ? '1' : '0';
    }
    buf[64] = '\0';
}

void print_interrupt_frame(struct interrupt_frame* frame) {
    char error_code_bin[64] = {0};
    uint64_to_binary_str(frame->error_code, error_code_bin);

    debug_print("=== Register State ===\n");
    debug_print("GS   = 0x%016lx  FS   = 0x%016lx  ES   = 0x%016lx  DS   = 0x%016lx\n",
              frame->gs, frame->fs, frame->es, frame->ds);
    debug_print("R15  = 0x%016lx  R14  = 0x%016lx  R13  = 0x%016lx  R12  = 0x%016lx\n",
              frame->r15, frame->r14, frame->r13, frame->r12);
    debug_print("R11  = 0x%016lx  R10  = 0x%016lx  R9   = 0x%016lx  R8   = 0x%016lx\n",
              frame->r11, frame->r10, frame->r9, frame->r8);
    debug_print("RDI  = 0x%016lx  RSI  = 0x%016lx  RBP  = 0x%016lx \n",
              frame->rdi, frame->rsi, frame->rbp);
    debug_print("RDX  = 0x%016lx  RCX  = 0x%016lx  RBX  = 0x%016lx  RAX  = 0x%016lx\n",
              frame->rdx, frame->rcx, frame->rbx, frame->rax);
    debug_print("Interrupt Number = %lu  Error Code = %lu (%s) (%016lX)\n", frame->interrupt_number, frame->error_code, error_code_bin, frame->error_code);
    debug_print("RIP  = 0x%016lx  CS   = 0x%016lx  RFLAGS = 0x%016lx\n",
              frame->rip, frame->cs, frame->rflags);
    debug_print("ORIG_RSP = 0x%016lx  SS   = 0x%016lx\n",
              frame->orig_rsp, frame->ss);
}

void interrupt_handler(struct interrupt_frame* frame) {
    // lets see..

    if (frame->interrupt_number >= 32) {
        // good interrupt.
        int cpu_id = get_lapic_id();

        struct interrupt_redirection_entry* entry = int_redir_table;
        while (entry) {
            if (entry->cpu_id == cpu_id && entry->idt_vector == frame->interrupt_number) {
                entry->handler(frame);
            }

            entry = entry->next;
        }

        lapic_send_eoi();

        return;
    }

    debug_print("Exception!\n");
    print_interrupt_frame(frame);

    if (frame->interrupt_number == 14) {
        print_page_fault_error_code(frame->error_code);
        // Print PTE info for the faulting address
        uint64_t cr2 = 0;
        asm volatile ("mov %%cr2, %0" : "=r"(cr2));
        extern void* get_current_pagemap();
        extern void vmem_print_pte_info(void*, uint64_t);
        vmem_print_pte_info(get_current_pagemap(), cr2);
    }

    while (true)
        asm volatile ("cli; hlt");
}

void idt_init() {
    // make idt entries point to our one point.
    for (int i = 0; i < 256; i++) {
        idt_entries[i] = create_idt_entry(0, 0x8, 0x0, false);
    }

    for (int i = 0; i < 256; i++) {
        idt_entries[i] = create_idt_entry(exception_handler_pointers[i], 0x8, 0x0, true);
    }

    idtr.base = (uint64_t) &idt_entries;
    idtr.limit = sizeof(idt_entries) - 1;

    // load the idt to the register
    flush_idt(&idtr);
}

extern void isr_common(struct interrupt_frame *frame);

void idt_set_handler(int vector, void (*handler)(struct interrupt_frame *)) {
    int current_cpu = get_lapic_id();

    // delete matching entries for this cpu and int vector.
    struct interrupt_redirection_entry **indirect = &int_redir_table;

    while (*indirect) {
        if ((*indirect)->cpu_id == current_cpu &&
            (*indirect)->idt_vector == vector) {
            struct interrupt_redirection_entry *to_free = *indirect;
            *indirect = to_free->next;
            free(to_free);
            } else {
                indirect = &(*indirect)->next;
            }
    }

    // add new handler if handler isnt NULL
    if (handler != NULL) {
        struct interrupt_redirection_entry *new_entry =
            (struct interrupt_redirection_entry *)malloc(sizeof(struct interrupt_redirection_entry));
        new_entry->cpu_id = current_cpu;
        new_entry->idt_vector = vector;
        new_entry->handler = handler;
        new_entry->next = int_redir_table;
        int_redir_table = new_entry;
    }
}
