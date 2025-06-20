//
// Created by neko on 6/7/25.
//

#ifndef IDT_H
#define IDT_H

#include <stdbool.h>
#include <stdint.h>

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct idt_entry {
    uint16_t offset0;          // offset bits 0..15
    uint16_t segment_selector; // code segment selector
    uint8_t ist;               // bits 0..2: IST, bits 3..7: zero
    uint8_t type_attr;         // type and attributes
    uint16_t offset1;          // offset bits 16..31
    uint32_t offset2;          // offset bits 32..63
    uint32_t zero;             // reserved
} __attribute__((packed));

struct interrupt_redirection_entry {
    int cpu_id;
    int idt_vector;
    void(*handler)(struct interrupt_frame*);
    struct interrupt_redirection_entry* next;
};

struct interrupt_frame
{
    uint64_t gs, fs, es, ds, r15, r14, r13, r12, r11,
        r10, r9, r8, rdi, rsi, rbp, rdx, rcx, rbx, rax, interrupt_number, error_code, rip, cs, rflags, orig_rsp,
        ss;
} __attribute__((packed));

void print_interrupt_frame(struct interrupt_frame* frame);
void interrupt_handler(struct interrupt_frame* frame);
extern void flush_idt(struct idtr*);
struct idt_entry create_idt_entry(uint64_t offset, uint16_t segment_selector, uint8_t dpl, bool present);
void idt_init();
void idt_set_handler(int vector, void (*handler)(struct interrupt_frame*));

#endif //IDT_H
