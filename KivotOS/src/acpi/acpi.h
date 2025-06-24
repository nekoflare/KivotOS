//
// Created by neko on 6/8/25.
//

#ifndef ACPI_H
#define ACPI_H

#include <limine.h>
#include <stddef.h>

// RSDP structure
struct rsdp
{
    char signature[8]; // RSD PTR
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address; // physical address to rsdt
    uint32_t length;       // unused in our case.

    // since revision >= 2
    uint64_t xsdt_address; // physical address
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

// SDT structure
struct sdt
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

// RSDT structure
struct rsdt
{
    struct sdt sdt;
    // n * 4 where n are (length - sizeof(sdt)) / 4
} __attribute__((packed));

// XSDT structure
struct xsdt
{
    struct sdt sdt;
    // n * 8 where n are (length - sizeof(sdt)) / 8
} __attribute__((packed));

// APIC table header
struct apic
{
    struct sdt sdt; // Common SDT header
    uint32_t local_controller_address;
    uint32_t flags;
    // Followed by APIC entries
} __attribute__((packed));

// MCFG table header
struct mcfg
{
    struct sdt sdt; // Common SDT header
    uint64_t reserved;
} __attribute__((packed));

struct mcfg_entry
{
    uint64_t base_address;
    uint16_t segment_group;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} __attribute__((packed));

// APIC entry types
enum ApicEntryType
{
    LOCAL_APIC                  = 0,
    IO_APIC                     = 1,
    INTERRUPT_SOURCE_OVERRIDE   = 2,
    NON_MASKABLE_INTERRUPT      = 4,
    LOCAL_APIC_ADDRESS_OVERRIDE = 5
};

// common APIC entry structure
struct apic_entry
{
    uint8_t type;
    uint8_t length;
    // data follow.
    // Variable-length entry
} __attribute__((packed));

// APIC-specific entry structures
struct local_apic_entry
{
    uint8_t type; // 0
    uint8_t length;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct io_apic_entry
{
    uint8_t type; // 1
    uint8_t length;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed));

// Interrupt Source Override (Type 2)
struct interrupt_source_override_entry
{
    uint8_t type; // 2
    uint8_t length;
    uint8_t bus;                      // Usually 0 (ISA)
    uint8_t source;                   // IRQ source
    uint32_t global_system_interrupt; // Global system interrupt
    uint16_t flags;
} __attribute__((packed));

// Non-Maskable Interrupt (Type 4)
struct non_maskable_interrupt_entry
{
    uint8_t type; // 4
    uint8_t length;
    uint8_t processor_id; // Processor ID (255 = all processors)
    uint16_t flags;
    uint8_t lint; // LINT# (0 or 1)
} __attribute__((packed));

void acpi_discover_xsdt_tables();
void acpi_discover_rsdt_tables();
void acpi_discover_tables();
void acpi_init();
void* acpi_get_table(char signature[4]);
void* acpi_get_physical_rsdp_address();
void acpi_further_init();

#endif //ACPI_H
