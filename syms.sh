#!/bin/bash
set -e

KERNEL_ELF="build/kernel.elf"
OUT_C="src/rt/kallsyms.c"
OUT_H="src/rt/kallsyms.h"

echo "[+] Generating kallsyms from $KERNEL_ELF"

# Extract all global text symbols
nm -n --defined-only "$KERNEL_ELF" \
  | grep ' T ' \
  | awk '{ printf "    { (uintptr_t)0x%s, \"%s\" },\n", $1, $3 }' \
  > .kallsyms_table.tmp

COUNT=$(wc -l < .kallsyms_table.tmp)

# Generate .h
cat <<EOF > "$OUT_H"
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
EOF

# Generate .c
cat <<EOF > "$OUT_C"
#include "kallsyms.h"

const kallsyms_entry_t kallsyms[] = {
$(cat .kallsyms_table.tmp)
};

const size_t kallsyms_count = $COUNT;

const char* kallsyms_lookup(uintptr_t addr) {
    for (size_t i = 0; i < kallsyms_count; i++) {
        if (kallsyms[i].address == addr)
            return kallsyms[i].name;
    }
    return "??";
}

const char* kallsyms_nearest(uintptr_t addr) {
    const char* result = "??";
    uintptr_t best = 0;
    for (size_t i = 0; i < kallsyms_count; i++) {
        if (kallsyms[i].address <= addr && kallsyms[i].address > best) {
            best = kallsyms[i].address;
            result = kallsyms[i].name;
        }
    }
    return result;
}
EOF

rm .kallsyms_table.tmp
echo "[+] Generated: $OUT_C ($COUNT entries)"
