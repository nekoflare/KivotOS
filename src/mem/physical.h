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

// Decorations
const char* limine_memory_map_type_to_string(uint64_t t);
void print_limine_memory_map_entry(struct limine_memmap_entry* e);
void print_free_physical_memory();

// Main functionalities, init, alloc, dealloc
void physical_memory_init();
uint64_t allocate_physical_page();
void deallocate_physical_page(uint64_t page);
uint64_t get_highest_valid_physical_address();
uint64_t allocate_physical_pages(size_t count);
void deallocate_physical_pages(uint64_t start_page, size_t count);
uint64_t get_physical_usable_memory_count();

#endif //PHYSICAL_H
