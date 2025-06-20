//
// Created by neko on 6/7/25.
//

#ifndef VIRTUAL_H
#define VIRTUAL_H

#include <stdint.h>
#include <limine.h>
#include <stddef.h>

struct virtual_address
{
    uint64_t canonical : 16;
    uint64_t pml4 : 9;
    uint64_t pdp : 9;
    uint64_t pd : 9;
    uint64_t pt : 9;
    uint64_t offset : 12;
} __attribute__((packed));

struct pml4
{
    uint64_t p : 1;        // present
    uint64_t rw : 1;       // read write
    uint64_t us : 1;       // user / supervisor
    uint64_t pwt : 1;      // page write through
    uint64_t pcd : 1;      // page cache disable
    uint64_t a : 1;        // accessed
    uint64_t ign1 : 1;     // ignored (bit 6)
    uint64_t zero : 1;     // must be zero (bit 7)
    uint64_t avl_low : 4;  // available (bits 8-11)
    uint64_t pdp_ppn : 40; // pdp page index (bits 12-51)
    uint64_t avl_high : 11;// available (bits 52-62)
    uint64_t nx : 1;       // no execute (bit 63)
} __attribute__((packed));

struct pdp
{
    uint64_t p : 1;       // present
    uint64_t rw : 1;      // read write
    uint64_t us : 1;      // user / supervisor
    uint64_t pwt : 1;     // page write through
    uint64_t pcd : 1;     // page cache disable
    uint64_t a : 1;       // accessed
    uint64_t ign0 : 1;    // ignored (bit 6)
    uint64_t zero : 1;    // must be zero (bit 7)
    uint64_t ign1 : 1;    // ignored (bit 8)
    uint64_t avl_low : 3; // available (bits 9-11)
    uint64_t pd_ppn : 40; // pd page index (bits 12-51)
    uint64_t avl_high : 11; // available (bits 52-62)
    uint64_t nx : 1;      // no execute (bit 63)
} __attribute__((packed));

struct pd
{
    uint64_t p : 1;       // present
    uint64_t rw : 1;      // read write
    uint64_t us : 1;      // user / supervisor
    uint64_t pwt : 1;     // page write through
    uint64_t pcd : 1;     // page cache disable
    uint64_t a : 1;       // accessed
    uint64_t ign0 : 1;    // ignored (bit 6)
    uint64_t zero : 1;    // must be zero (bit 7)
    uint64_t ign1 : 1;    // ignored (bit 8)
    uint64_t avl_low : 3; // available (bits 9-11)
    uint64_t pt_ppn : 40; // pt page index (bits 12-51)
    uint64_t avl_high : 11; // available (bits 52-62)
    uint64_t nx : 1;      // no execute (bit 63)
} __attribute__((packed));

struct pt
{
    uint64_t p : 1;         // present
    uint64_t rw : 1;        // read write
    uint64_t us : 1;        // user / supervisor
    uint64_t pwt : 1;       // page write through
    uint64_t pcd : 1;       // page cache disable
    uint64_t a : 1;         // accessed
    uint64_t d : 1;         // dirty
    uint64_t pat : 1;       // page attribute table
    uint64_t g : 1;         // global
    uint64_t avl_low : 3;   // available (bits 9-11)
    uint64_t phys_ppn : 40; // physical address page index (bits 12-51)
    uint64_t avl_mid : 7;   // available (bits 52-58)
    uint64_t pk : 4;        // protection keys (bits 59-62)
    uint64_t nx : 1;        // no execute (bit 63)
} __attribute__((packed));

#define FLAG_RW (1 << 0)
#define FLAG_US (1 << 1)
#define FLAG_PWT (1 << 2)
#define FLAG_PCD (1 << 3)
#define FLAG_PAT (1 << 4)
#define FLAG_NX (1 << 5)

void vmem_init();
uint64_t get_hhdm_slide();
uint64_t vmem_allocate_memory(uint64_t len);
void vmem_free_memory(uint64_t address, uint64_t len);
void vmem_map(void* pagemap, uint64_t virt, uint64_t phys, uint64_t len, uint64_t flags);
void vmem_unmap(void* pagemap, uint64_t virt, uint64_t len);
void unmap_page(void *pagemap, uint64_t virt);
void* get_kernel_pagemap();

#endif //VIRTUAL_H