//
// Created by neko on 6/7/25.
//

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

struct gdtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct gdt_entry {
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t access;
    uint8_t limit1 : 4;
    uint8_t flags : 4;
    uint8_t base2;
} __attribute__((packed));

struct tss {
    uint32_t reserved1;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomap_base;
} __attribute__((packed));
#define GDT_ACC_A           (1 << 0)
#define GDT_ACC_RW          (1 << 1)
#define GDT_ACC_DC          (1 << 2)
#define GDT_ACC_X           (1 << 3)
#define GDT_ACC_S           (1 << 4)
#define GDT_ACC_DPL(dpl)    (((dpl) & 0x3) << 5)
#define GDT_ACC_P           (1 << 7)

#define GDT_FLAG_LM (1 << 1)
#define GDT_FLAG_DB (1 << 2)
#define GDT_FLAG_G  (1 << 3)

extern void flush_gdt(uint16_t code_segment_selector, uint16_t data_segment_selector);
struct gdt_entry create_gdt_entry(uint32_t base, uint32_t limit, uint8_t flags, uint8_t access);
void gdt_init();
void tss_init();

#endif //GDT_H
