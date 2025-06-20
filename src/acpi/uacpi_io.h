//
// Created by neko on 6/17/25.
//

#ifndef UACPI_IO_H
#define UACPI_IO_H

#include <stdint.h>

struct io_region {
    uint16_t base;
    uint16_t length;
};

struct io_region* create_io_region(uint16_t base, uint16_t length);
void free_io_region(struct io_region *region);

uint8_t ior_read8(struct io_region *region, uint16_t offset);
uint16_t ior_read16(struct io_region *region, uint16_t offset);
uint32_t ior_read32(struct io_region *region, uint16_t offset);

void ior_write8(struct io_region *region, uint16_t offset, uint8_t value);
void ior_write16(struct io_region *region, uint16_t offset, uint16_t value);
void ior_write32(struct io_region *region, uint16_t offset, uint32_t value);

#endif //UACPI_IO_H
