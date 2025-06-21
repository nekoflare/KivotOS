//
// Created by neko on 6/7/25.
//

#include "virtual.h"

#include <stdbool.h>
#include <string.h>
#include <x86/log.h>
#include <rt/mutex.h>

#include "physical.h"

volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = LIMINE_API_REVISION,
    .response = NULL
};

volatile struct limine_executable_address_request ea_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .revision = LIMINE_API_REVISION,
    .response = NULL
};

uint64_t vmem_bottom = 0;
uint64_t vmem_left = 0;
void* kernel_pagemap = NULL;

static struct mutex vmem_mutex = {0};

struct virtual_address split_virtual_address_to_structure(const uint64_t address)
{
    struct virtual_address result;

    result.canonical = (address >> 48) & 0xFFFF;
    result.pml4 = (address >> 39) & 0x1FF;
    result.pdp = (address >> 30) & 0x1FF;
    result.pd = (address >> 21) & 0x1FF;
    result.pt = (address >> 12) & 0x1FF;
    result.offset = address & 0xFFF;

    return result;
}

void vmem_init() {
    lock_mutex(&vmem_mutex);
    vmem_bottom = get_hhdm_slide() + pmem_get_highest_address();
    vmem_left = ea_request.response->virtual_base - vmem_bottom;

    uint64_t cr3 = 0;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    cr3 &= 0xfffffffffffff000;                      // get rid of flags
    cr3 += get_hhdm_slide();
    kernel_pagemap = (void*) cr3;

    debug_print("Available virtual memory: %lu (B)", vmem_left);
    unlock_mutex(&vmem_mutex);
}

uint64_t get_hhdm_slide() {
    return hhdm_request.response->offset; // todo: if in debug mode check if this is null.
}

uint64_t vmem_allocate_memory(uint64_t len) {
    if (len % 4096 != 0) {
        debug_print("Len %% 4096 != 0. in virtual.c\n");
        asm volatile ("cli; hlt");
    }

    lock_mutex(&vmem_mutex);
    uint64_t allocated = vmem_bottom + len;
    vmem_bottom += len;
    unlock_mutex(&vmem_mutex);
    return allocated;
}

void vmem_free_memory(uint64_t address, uint64_t len) {
    lock_mutex(&vmem_mutex);
    debug_print("vmem_free_memory not implemented. not critical but do me someday.\n");
    unlock_mutex(&vmem_mutex);
}

void* get_current_pagemap() {
    uint64_t cr3 = 0;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return (void*) ((cr3 & 0xfffffffffffff000));
}

void load_pagemap(void* pm) {
    uint64_t cr3 = 0;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    cr3 &= 0xfff; // get rid of everything **but** flags.
    cr3 |= (uint64_t) (pm); // time to lock in
    debug_print("Setting CR3 to 0x%016lX\n", cr3);
    asm volatile("mov %0, %%cr3" :: "r"(cr3));
}

void map_page(void* pagemap, uint64_t virt, uint64_t phys, uint64_t flags) {
    if (pagemap == NULL) {
        pagemap = (void*)((uintptr_t)get_current_pagemap() + get_hhdm_slide());
    } else {
        pagemap = (void*)((uintptr_t)pagemap + get_hhdm_slide());
    }

    uintptr_t hhdm = get_hhdm_slide();

    struct virtual_address vaddr = split_virtual_address_to_structure(virt);

    struct pml4* pml4_table = (struct pml4*)pagemap;
    struct pdp* pdp_table;
    struct pd* pd_table;
    struct pt* pt_table;
    struct pt* entry;

    // Extract flags
    int rw  = (flags & FLAG_RW)  ? 1 : 0;
    int us  = (flags & FLAG_US)  ? 1 : 0;
    int pwt = (flags & FLAG_PWT) ? 1 : 0;
    int pcd = (flags & FLAG_PCD) ? 1 : 0;
    int pat = (flags & FLAG_PAT) ? 1 : 0;
    int nx  = (flags & FLAG_NX)  ? 1 : 0;

    // Allocate PDP if needed
    if (pml4_table[vaddr.pml4].pdp_ppn == 0) {
        uint64_t pdp_phys = page_alloc();
        if (!pdp_phys) {
            return;
        }
        memset((void*)(pdp_phys + hhdm), 0, 0x1000);
        pml4_table[vaddr.pml4].pdp_ppn = pdp_phys >> 12;
    }
    // Set PML4E flags
    pml4_table[vaddr.pml4].p = 1;
    pml4_table[vaddr.pml4].rw = rw;
    pml4_table[vaddr.pml4].us = us;
    pml4_table[vaddr.pml4].nx = nx;

    pdp_table = (struct pdp*)((pml4_table[vaddr.pml4].pdp_ppn << 12) + hhdm);

    // Allocate PD if needed
    if (pdp_table[vaddr.pdp].pd_ppn == 0) {
        uint64_t pd_phys = page_alloc();
        if (!pd_phys) {
            return;
        }
        memset((void*)(pd_phys + hhdm), 0, 0x1000);
        pdp_table[vaddr.pdp].pd_ppn = pd_phys >> 12;
    }
    // Set PDPE flags
    pdp_table[vaddr.pdp].p = 1;
    pdp_table[vaddr.pdp].rw = rw;
    pdp_table[vaddr.pdp].us = us;
    pdp_table[vaddr.pdp].nx = nx;

    pd_table = (struct pd*)((pdp_table[vaddr.pdp].pd_ppn << 12) + hhdm);

    // Allocate PT if needed
    if (pd_table[vaddr.pd].pt_ppn == 0) {
        uint64_t pt_phys = page_alloc();
        if (!pt_phys) {
            return;
        }
        memset((void*)(pt_phys + hhdm), 0, 0x1000);
        pd_table[vaddr.pd].pt_ppn = pt_phys >> 12;
    }
    // Set PDE flags
    pd_table[vaddr.pd].p = 1;
    pd_table[vaddr.pd].rw = rw;
    pd_table[vaddr.pd].us = us;
    pd_table[vaddr.pd].nx = nx;

    pt_table = (struct pt*)((pd_table[vaddr.pd].pt_ppn << 12) + hhdm);
    entry = &pt_table[vaddr.pt];

    // Check if already mapped
    if (entry->p) {
        debug_print("[map_page] ERROR: Attempt to map over existing PTE for virt=0x%016lX (phys=0x%016lX, old phys=0x%016lX)\n", virt, phys, (uint64_t)entry->phys_ppn << 12);
        asm volatile ("cli; hlt");
        return;
    }

    // Write PTE
    entry->phys_ppn = phys >> 12;
    entry->p = 1;
    entry->rw = rw;
    entry->us = us;
    entry->pwt = pwt;
    entry->pcd = pcd;
    entry->pat = pat;
    entry->nx = nx;

    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}


void vmem_map(void* pagemap, uint64_t virt, uint64_t phys, uint64_t len, uint64_t flags) {
    if (virt % 4096 != 0) {
        debug_print("virt isn't aligned.\n");
        asm volatile ("cli; hlt");
    }

    if (phys % 4096 != 0) {
        debug_print("phys = %016lX\n", phys);
        debug_print("phys isn't aligned.\n");
        asm volatile ("cli; hlt");
    }

    if (len % 4096 != 0) {
        debug_print("len isn't aligned.\n");
        asm volatile ("cli; hlt");
    }

    for (uint64_t offset = 0; offset < len; offset += 4096) {
        map_page(pagemap, virt + offset, phys + offset, flags);
    }
}

void unmap_page(void *pagemap, uint64_t virt) {
    if (pagemap == NULL) {
        pagemap = get_current_pagemap();
    }

    uintptr_t hhdm = get_hhdm_slide();
    struct virtual_address vaddr = split_virtual_address_to_structure(virt);

    struct pml4 *pml4_table = (struct pml4 *) pagemap;
    if (!pml4_table[vaddr.pml4].p) return;

    struct pdp *pdp_table = (struct pdp *) ((pml4_table[vaddr.pml4].pdp_ppn << 12) + hhdm);
    if (!pdp_table[vaddr.pdp].p) return;

    struct pd *pd_table = (struct pd *) ((pdp_table[vaddr.pdp].pd_ppn << 12) + hhdm);
    if (!pd_table[vaddr.pd].p) return;

    struct pt *pt_table = (struct pt *) ((pd_table[vaddr.pd].pt_ppn << 12) + hhdm);
    struct pt *entry = &pt_table[vaddr.pt];

    memset(entry, 0, sizeof(struct pt));
    
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmem_unmap(void *pagemap, uint64_t virt, uint64_t len) {
    if (virt % 4096 != 0) {
        debug_print("virt isn't aligned.\n");
        asm volatile ("cli; hlt");
    }

    if (len % 4096 != 0) {
        debug_print("len isn't aligned.\n");
        asm volatile ("cli; hlt");
    }

    for (uint64_t offset = 0; offset < len; offset += 4096) {
        unmap_page(pagemap, virt + offset);
    }
}

void *get_kernel_pagemap() {
    return kernel_pagemap;
}

void vmem_print_pte_info(void* pagemap, uint64_t vaddr) {
    if (pagemap == NULL) {
        pagemap = get_current_pagemap();
    }
    uintptr_t hhdm = get_hhdm_slide();
    struct virtual_address va = split_virtual_address_to_structure(vaddr);
    struct pml4 *pml4_table = (struct pml4 *)pagemap;
    struct pdp *pdp_table;
    struct pd *pd_table;
    struct pt *pt_table;
    debug_print("\n--- vmem_print_pte_info ---\n");
    debug_print("Virtual address: 0x%016lX\n", vaddr);
    uint64_t pml4e = *((uint64_t*)&pml4_table[va.pml4]);
    debug_print("PML4[%lu] @ %p: 0x%016lX\n", va.pml4, &pml4_table[va.pml4], pml4e);
    debug_print("  Flags: %s%s%s%s%s%s%s%s\n",
        pml4_table[va.pml4].p ? "P " : "",
        pml4_table[va.pml4].rw ? "RW " : "",
        pml4_table[va.pml4].us ? "US " : "",
        pml4_table[va.pml4].pwt ? "PWT " : "",
        pml4_table[va.pml4].pcd ? "PCD " : "",
        pml4_table[va.pml4].nx ? "NX " : "",
        pml4_table[va.pml4].a ? "A " : "",
        "");
    if (!pml4_table[va.pml4].p) return;
    pdp_table = (struct pdp *)((pml4_table[va.pml4].pdp_ppn << 12) + hhdm);
    uint64_t pdpe = *((uint64_t*)&pdp_table[va.pdp]);
    debug_print("PDP [%lu] @ %p: 0x%016lX\n", va.pdp, &pdp_table[va.pdp], pdpe);
    debug_print("  Flags: %s%s%s%s%s%s%s%s\n",
        pdp_table[va.pdp].p ? "P " : "",
        pdp_table[va.pdp].rw ? "RW " : "",
        pdp_table[va.pdp].us ? "US " : "",
        pdp_table[va.pdp].pwt ? "PWT " : "",
        pdp_table[va.pdp].pcd ? "PCD " : "",
        pdp_table[va.pdp].nx ? "NX " : "",
        pdp_table[va.pdp].a ? "A " : "",
        "");
    if (!pdp_table[va.pdp].p) return;
    pd_table = (struct pd *)((pdp_table[va.pdp].pd_ppn << 12) + hhdm);
    uint64_t pde = *((uint64_t*)&pd_table[va.pd]);
    debug_print("PD  [%lu] @ %p: 0x%016lX\n", va.pd, &pd_table[va.pd], pde);
    debug_print("  Flags: %s%s%s%s%s%s%s%s\n",
        pd_table[va.pd].p ? "P " : "",
        pd_table[va.pd].rw ? "RW " : "",
        pd_table[va.pd].us ? "US " : "",
        pd_table[va.pd].pwt ? "PWT " : "",
        pd_table[va.pd].pcd ? "PCD " : "",
        pd_table[va.pd].nx ? "NX " : "",
        pd_table[va.pd].a ? "A " : "",
        "");
    if (!pd_table[va.pd].p) return;
    pt_table = (struct pt *)((pd_table[va.pd].pt_ppn << 12) + hhdm);
    uint64_t pte = *((uint64_t*)&pt_table[va.pt]);
    debug_print("PT  [%lu] @ %p: 0x%016lX\n", va.pt, &pt_table[va.pt], pte);
    debug_print("  Flags: %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
        pt_table[va.pt].p ? "P " : "",
        pt_table[va.pt].rw ? "RW " : "",
        pt_table[va.pt].us ? "US " : "",
        pt_table[va.pt].pwt ? "PWT " : "",
        pt_table[va.pt].pcd ? "PCD " : "",
        pt_table[va.pt].a ? "A " : "",
        pt_table[va.pt].d ? "D " : "",
        pt_table[va.pt].pat ? "PAT " : "",
        pt_table[va.pt].g ? "G " : "",
        pt_table[va.pt].nx ? "NX " : "",
        pt_table[va.pt].pk ? "PK " : "",
        "",
        "");
    debug_print("--------------------------\n");
}

