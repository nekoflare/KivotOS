//
// Created by neko on 6/17/25.
//

#include "uacpi_io.h"

#include <stdlib.h>
#include <x86/io.h>

struct io_region *create_io_region(uint16_t base, uint16_t length) {
    struct io_region *region = malloc(sizeof(struct io_region));
    region->base = base;
    region->length = length;
    return region;
}

void free_io_region(struct io_region *region) {
    free(region);
}

uint8_t ior_read8(struct io_region *region, uint16_t offset) {
    return inb(region->base + offset);
}

uint16_t ior_read16(struct io_region *region, uint16_t offset) {
    return inw(region->base + offset);
}

uint32_t ior_read32(struct io_region *region, uint16_t offset) {
    return inl(region->base + offset);
}

void ior_write8(struct io_region *region, uint16_t offset, uint8_t value) {
    outb(region->base + offset, value);
}

void ior_write16(struct io_region *region, uint16_t offset, uint16_t value) {
    outw(region->base + offset, value);
}

void ior_write32(struct io_region *region, uint16_t offset, uint32_t value) {
    outl(region->base + offset, value);
}

