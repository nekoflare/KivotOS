#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uintptr_t address;
    const char* name;
} kallsyms_entry_t;

extern const kallsyms_entry_t kallsyms[];
extern const size_t kallsyms_count;

const char* kallsyms_lookup(uintptr_t addr);
const char* kallsyms_nearest(uintptr_t addr);
