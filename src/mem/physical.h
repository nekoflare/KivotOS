//
// Created by neko on 6/7/25.
//
// @note: Wont work in multithreaded envs
//

#ifndef PHYSICAL_H
#define PHYSICAL_H

#include <limine.h>
#include <stdint.h>
#include <stddef.h>
#include <x86/log.h>

struct pmem_freelist_entry {
    struct pmem_freelist_entry* next;
    uint64_t length;
};

// Freelist management
void pmem_push_to_freelist(struct pmem_freelist_entry* entry);

// Decorations
const char* pmem_limine_memmap_type_to_string(uint64_t t);
void pmem_print_memmap_entry(struct limine_memmap_entry* e);
void pmem_count_free_memory_and_print();

// Main functionalities, init, alloc, dealloc
void pmem_init();
uint64_t page_alloc();
void page_dealloc(uint64_t page);
uint64_t pmem_get_highest_address();
uint64_t page_alloc_pages(size_t count);
void page_dealloc_pages(uint64_t start_page, size_t count);
uint64_t pmem_get_usable_memory_count();

#endif //PHYSICAL_H
