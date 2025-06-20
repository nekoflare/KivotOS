//
// Created by neko on 6/8/25.
//


#include "gheap.h"
#include "dlmalloc.h"

#include <unistd.h>
#include <stddef.h>
#include <sys/mman.h>
#include <stdint.h>
#include <x86/log.h>
#include <rt/mutex.h>

#include "physical.h"
#include "virtual.h"

uint64_t heap_base = 0;
uint64_t current_heap_size = 0;

static struct mutex gheap_alloc_mutex = {0};
static struct mutex gheap_mutex = {};

void* malloc(size_t n) {
    lock_mutex(&gheap_alloc_mutex);
    void* ret = dlmalloc(n);
    unlock_mutex(&gheap_alloc_mutex);
    return ret;
}

void free(void* ptr) {
    lock_mutex(&gheap_alloc_mutex);
    dlfree(ptr);
    unlock_mutex(&gheap_alloc_mutex);
}

void gheap_init() {
    lock_mutex(&gheap_mutex);
    heap_base = vmem_allocate_memory(HEAP_SIZE);
    if (!heap_base) {
        debug_print("Failed to allocate virtual heap base.\n");
        asm volatile ("cli; hlt");
    }
    unlock_mutex(&gheap_mutex);
}

void *sbrk(ptrdiff_t increment) {
    lock_mutex(&gheap_mutex);
    if (increment == 0) {
        void* ret = (void*)(heap_base + current_heap_size);
        unlock_mutex(&gheap_mutex);
        return ret;
    }

    increment = ALIGN_UP(increment, 4096);

    if (current_heap_size + increment > HEAP_SIZE) {
        debug_print("Heap size exceeds limit.\n");
        unlock_mutex(&gheap_mutex);
        asm volatile ("cli; hlt");
    }

    void *old_break = (void*)(heap_base + current_heap_size);
    uint64_t phys = page_alloc_pages(increment / 4096);

    if (phys) {
        vmem_map(NULL, (uint64_t)old_break, phys, increment, FLAG_RW | FLAG_US);
        current_heap_size += increment;
    } else {
        for (size_t i = 0; i < increment; i += 4096) {
            uint64_t page = page_alloc();
            if (!page) {
                debug_print("Out of physical pages in sbrk fallback.\n");
                unlock_mutex(&gheap_mutex);
                asm volatile ("cli; hlt");
            }

            vmem_map(NULL, heap_base + current_heap_size, page, 4096, FLAG_RW | FLAG_US);
            current_heap_size += 4096;
        }
    }

    unlock_mutex(&gheap_mutex);
    return old_break;
}

long sysconf(int name) {
    switch (name) {
        case _SC_AVPHYS_PAGES: {
            uint64_t available_physical_memory = pmem_get_usable_memory_count();
            return (long)(available_physical_memory / 4096);
        }
        case _SC_PAGE_SIZE:
            return 4096;
        default:
            debug_print("Unknown name %d in sysconf.\n", name);
            asm volatile ("cli; hlt");
            return -1; // unreachable
    }
}

extern void *mmap(void *__addr, size_t __len, int __prot,
                  int __flags, int __fd, __off_t __offset) {

    lock_mutex(&gheap_mutex);
    if (__len == 0) {
        debug_print("mmap() called with size 0.\n");
        unlock_mutex(&gheap_mutex);
        return (void*)-1;
    }

    size_t aligned_size = ALIGN_UP(__len, 4096);
    uint64_t vmem = vmem_allocate_memory(aligned_size);
    if (!vmem) {
        debug_print("mmap: out of virtual memory space.\n");
        unlock_mutex(&gheap_mutex);
        return (void*)-1;
    }

    int flags = 0;
    if (__prot & PROT_READ) flags |= FLAG_US;
    if (__prot & PROT_WRITE) flags |= FLAG_RW;
    if (__prot & PROT_EXEC) {
    }

    size_t num_pages = aligned_size / 4096;
    for (size_t i = 0; i < num_pages; i++) {
        uint64_t page = page_alloc();
        if (!page) {
            debug_print("mmap: out of physical memory.\n");
            unlock_mutex(&gheap_mutex);
            return (void*)-1;
        }

        vmem_map(NULL, vmem + i * 4096, page, 4096, flags);
    }

    unlock_mutex(&gheap_mutex);
    return (void*)vmem;
}

int munmap(void *__addr, size_t __len) {
    lock_mutex(&gheap_mutex);
    debug_print("munmap not implemented.\n");
    unlock_mutex(&gheap_mutex);
    return 0;
}
