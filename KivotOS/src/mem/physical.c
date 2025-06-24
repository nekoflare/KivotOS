//
// Created by neko on 6/7/25.
//

#include "physical.h"

#include <rt/mutex.h>

#include "virtual.h"

volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = LIMINE_API_REVISION,
    .response = NULL
};

struct free_list_entry_t {
    struct free_list_entry_t* next;
    uint64_t length;
};

struct mutex pmem_freelist_entry_mutex = {};
struct free_list_entry_t* head = NULL;

void push_physical_memory_area_to_free_list(struct free_list_entry_t* entry) {
    lock_mutex(&pmem_freelist_entry_mutex);
    if (head == NULL) {
        head = entry;
        unlock_mutex(&pmem_freelist_entry_mutex);
        return;
    }

    entry->next = head;
    head = entry;
    unlock_mutex(&pmem_freelist_entry_mutex);
}

const char* limine_memory_map_type_to_string(uint64_t t) {
    switch (t) {
        case LIMINE_MEMMAP_USABLE: return "Usable";
        case LIMINE_MEMMAP_RESERVED: return "Reserved";
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE: return "ACPI Reclaimable";
        case LIMINE_MEMMAP_ACPI_NVS: return "ACPI NVS";
        case LIMINE_MEMMAP_BAD_MEMORY: return "Bad Memory";
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: return "Bootloader Reclaimable";
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES: return "Executable Modules";
        case LIMINE_MEMMAP_FRAMEBUFFER: return "Framebuffer";
        default: return "Unknown entry";
    }
}

void print_limine_memory_map_entry(struct limine_memmap_entry* e) {
    debug_print("Memory map entry | Base: %016llX Length: %08lu (B) Type: %s\n", e->base, e->length, limine_memory_map_type_to_string(e->type));
}

void print_free_physical_memory() {
    lock_mutex(&pmem_freelist_entry_mutex);
    struct free_list_entry_t* entry = head;
    size_t free_mem = 0;

    while (entry) {
        free_mem += entry->length;
        entry = entry->next;
    }

    debug_print("Free memory: %lu (B)\n", free_mem);
    unlock_mutex(&pmem_freelist_entry_mutex);
}

void pmem_print_freelist() {
    lock_mutex(&pmem_freelist_entry_mutex);
    struct free_list_entry_t *entry = head;
    debug_print("Physical memory freelist:\n");

    while (entry) {
        debug_print("Entry at %016llX, length: %lu bytes\n",
                    (uint64_t) entry - get_hhdm_slide(), entry->length);
        entry = entry->next;
    }
    unlock_mutex(&pmem_freelist_entry_mutex);
}

void physical_memory_init() {
    if (memmap_request.response == NULL) {
        debug_print("No memory map.\n");
        asm volatile ("cli; hlt");
    }

    struct limine_memmap_entry** entries = memmap_request.response->entries;
    const uint64_t entry_count = memmap_request.response->entry_count;

    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry* entry = entries[i];

        print_limine_memory_map_entry(entry);

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            // add it to the freelist.
            struct free_list_entry_t* ent = (struct free_list_entry_t*)(get_hhdm_slide() + entry->base);
            ent->length = entry->length;
            ent->next = NULL;

            push_physical_memory_area_to_free_list(ent);
        }
    }

    print_free_physical_memory();
    pmem_print_freelist();
}

uint64_t allocate_physical_page() {
    lock_mutex(&pmem_freelist_entry_mutex);

    if (head == NULL || head->length < 4096) {
        unlock_mutex(&pmem_freelist_entry_mutex);
        return 0;
    }

    struct free_list_entry_t* this = head;
    uint64_t addr = (uint64_t)this - get_hhdm_slide(); // physical address to return

    if (head->length == 4096) {
        // remove entry from freelist
        head = head->next;
    } else {
        // shrink current freelist entry
        head = (struct free_list_entry_t*)((char*)this + 4096);
        head->length = this->length - 4096;
        head->next = this->next;
    }
    unlock_mutex(&pmem_freelist_entry_mutex);
    return addr;
}

void deallocate_physical_page(uint64_t page) {
    if (page == 0) return; // invalid page

    struct free_list_entry_t* entry = (struct free_list_entry_t*)(page + get_hhdm_slide());
    entry->length = 4096;
    entry->next = NULL;

    lock_mutex(&pmem_freelist_entry_mutex);

    if (head == NULL) {
        head = entry;
        unlock_mutex(&pmem_freelist_entry_mutex);
        return;
    }

    struct free_list_entry_t* prev = NULL;
    struct free_list_entry_t* current = head;

    while (current && current < entry) {
        prev = current;
        current = current->next;
    }

    entry->next = current;

    if (prev == NULL) {
        head = entry;
    } else {
        prev->next = entry;
    }

    // try to coalesce with the next block if adjacent
    if (entry->next &&
        (uint64_t)entry + entry->length == (uint64_t)entry->next) {
        entry->length += entry->next->length;
        entry->next = entry->next->next;
    }

    // try to coalesce with the previous block if adjacent
    if (prev &&
        (uint64_t)prev + prev->length == (uint64_t)entry) {
        prev->length += entry->length;
        prev->next = entry->next;
    }

    unlock_mutex(&pmem_freelist_entry_mutex);
}

uint64_t get_highest_valid_physical_address() {
    lock_mutex(&pmem_freelist_entry_mutex);

    if (memmap_request.response == NULL) {
        debug_print("No memory map.\n");
        asm volatile ("cli; hlt");
    }

    struct limine_memmap_entry** entries = memmap_request.response->entries;
    const uint64_t entry_count = memmap_request.response->entry_count;

    uint64_t highest_address = 0;

    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry* entry = entries[i];

        if (entry->base + entry->length > highest_address) {
            highest_address = entry->base + entry->length;
        }
    }

    unlock_mutex(&pmem_freelist_entry_mutex);
    return highest_address;
}

uint64_t allocate_physical_pages(size_t count) {
    lock_mutex(&pmem_freelist_entry_mutex);

    if (count == 0) return 0;
    size_t needed_bytes = count * 4096;

    struct free_list_entry_t* prev = NULL;
    struct free_list_entry_t* current = head;

    while (current) {
        if (current->length >= needed_bytes) {
            uint64_t allocated_address = (uint64_t)current - get_hhdm_slide();

            if (current->length == needed_bytes) {
                // exact fit
                if (prev) prev->next = current->next;
                else head = current->next;
            } else {
                // save old next pointer before splitting
                struct free_list_entry_t* old_next = current->next;
                size_t old_length = current->length;

                // create new block after allocated region
                struct free_list_entry_t* new_block =
                    (struct free_list_entry_t*)((uint64_t)current + needed_bytes);
                new_block->length = old_length - needed_bytes;
                new_block->next = old_next;

                // update list pointers
                if (prev) prev->next = new_block;
                else head = new_block;
            }
            unlock_mutex(&pmem_freelist_entry_mutex);
            return allocated_address;
        }
        prev = current;
        current = current->next;
    }
    unlock_mutex(&pmem_freelist_entry_mutex);
    return 0; // no suitable block found :((((
}

void deallocate_physical_pages(uint64_t start_page, size_t count) {
    lock_mutex(&pmem_freelist_entry_mutex);

    if (count == 0 || start_page == 0) return;

    struct free_list_entry_t* entry = (struct free_list_entry_t*)(start_page + get_hhdm_slide());
    entry->length = count * 4096;
    entry->next = NULL;

    if (head == NULL) {
        head = entry;
        unlock_mutex(&pmem_freelist_entry_mutex);
        return;
    }

    // insert into freelist sorted by address
    struct free_list_entry_t* prev = NULL;
    struct free_list_entry_t* current = head;

    while (current && current < entry) {
        prev = current;
        current = current->next;
    }

    entry->next = current;

    if (prev == NULL) {
        head = entry;
    } else {
        prev->next = entry;
    }

    // coalesce with next if adjacent
    if (entry->next && (uint64_t)entry + entry->length == (uint64_t)entry->next) {
        entry->length += entry->next->length;
        entry->next = entry->next->next;
    }

    // coalesce with previous if adjacent
    if (prev && (uint64_t)prev + prev->length == (uint64_t)entry) {
        prev->length += entry->length;
        prev->next = entry->next;
    }

    unlock_mutex(&pmem_freelist_entry_mutex);
}

uint64_t get_physical_usable_memory_count() {
    lock_mutex(&pmem_freelist_entry_mutex);
    struct free_list_entry_t* entry = head;
    size_t free_mem = 0;

    while (entry) {
        free_mem += entry->length;
        entry = entry->next;
    }

    unlock_mutex(&pmem_freelist_entry_mutex);
    return free_mem;
}
