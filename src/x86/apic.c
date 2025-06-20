//
// Created by neko on 6/8/25.
//

#include "apic.h"

#include <stdbool.h>
#include <stdlib.h>
#include <acpi/acpi.h>
#include <mem/virtual.h>
#include <x86/msr.h>

#include "idt.h"
#include "io.h"
#include "log.h"

struct lapic_linked_list_entry {
    struct lapic_linked_list_entry* next;
    struct local_apic_entry* local_apic;
};

struct ioapic_linked_list_entry {
    struct ioapic_linked_list_entry* next;
    struct io_apic_entry* io_apic;
};

struct interrupt_source_override_linked_list_entry {
    struct interrupt_source_override_linked_list_entry* next;
    struct interrupt_source_override_entry* interrupt_source_override;
};

struct ioapic_entry
{
    uint8_t vector;               // 8 bits: Interrupt vector (0-7)
    uint8_t delivery_mode : 3;    // 3 bits: Delivery Mode (8-10)
    uint8_t destination_mode : 1; // 1 bit: Destination Mode (11)
    uint8_t delivery_status : 1;  // 1 bit: Delivery Status (12)
    uint8_t pin_polarity : 1;     // 1 bit: Pin Polarity (13)
    uint8_t remote_irr : 1;       // 1 bit: Remote IRR (14)
    uint8_t trigger_mode : 1;     // 1 bit: Trigger Mode (15)
    uint8_t mask : 1;             // 1 bit: Mask (16)
    uint32_t reserved0 : 32;      // 39 bits reserved (17-55) - Using uint8_t for the
    // first part of reserved
    uint8_t reserved1 : 7;
    uint8_t destination; // 8 bits: Destination (56-63)
} __attribute__((packed));

struct cpu_vectors {
    uint8_t vectors[256 / (sizeof(uint8_t) * 8)]; // free vectors on the cpu of IDT
};

struct cpu_irq_info_entry {
    struct cpu_irq_info_entry* next;
    int gsi;
    int vector;

    void (*handler)();
};

struct cpu_local_info {
    struct cpu_vectors vectors;
    struct cpu_irq_info_entry* head;
};

struct cpu_local_info cpu_local_info[MAX_CPUS] = {0};
struct lapic_linked_list_entry* lapic_linked_list_head = NULL;
struct ioapic_linked_list_entry* ioapic_linked_list_head = NULL;
struct interrupt_source_override_linked_list_entry* interrupt_source_override_linked_list_head = NULL;

uint32_t apic_read(const uintptr_t apic_base, const uint32_t offset)
{
    volatile uint32_t* register_address = (volatile uint32_t*)(apic_base + offset);
    return *register_address;
}

// Write to an APIC register
void apic_write(uintptr_t apic_base, const uint32_t offset, const uint32_t value)
{
    volatile uint32_t* register_address = (volatile uint32_t*)(apic_base + offset);
    *register_address = value;
}

uint64_t get_lapic_address()
{
    uint64_t lapic_address = rdmsr(IA32_APIC_BASE);

    lapic_address &= ~(1 << 11); // apic global enable flag, disable it, we need address
    lapic_address &= ~(1 << 8); // bsp flag, disable it, we need address cuh

    lapic_address += get_hhdm_slide(); // make it virtual cuh

    return lapic_address;
}

uint32_t get_lapic_id()
{
    return apic_read(get_lapic_address(), APIC_REGISTER_ID);
}

void ioapic_write(uintptr_t ioapic_base, uint32_t reg, uint32_t value)
{
    volatile uint32_t *ioapic_index = (volatile uint32_t*)(ioapic_base);
    volatile uint32_t *ioapic_data = (volatile uint32_t*)(ioapic_base + 0x10);

    *ioapic_index = reg;
    *ioapic_data = value;
}

uint32_t ioapic_read(uintptr_t ioapic_base, uint32_t reg)
{
    volatile uint32_t *ioapic_index = (volatile uint32_t*)(ioapic_base);
    volatile uint32_t *ioapic_data = (volatile uint32_t*)(ioapic_base + 0x10);

    *ioapic_index = reg;
    return *ioapic_data;
}

void ioapic_set_redirection(uintptr_t ioapic_base, uint8_t irq, uint64_t destination, uint8_t vector,
                            uint8_t delivery_mode, bool level_triggered, bool physical_destination,
                            bool delivery_status, bool pin_polarity, bool mask_irq)
{
    uint32_t low_index = IOAPIC_REGISTER_REDIRECTION_BASE + (irq * 2);
    uint32_t high_index = IOAPIC_REGISTER_REDIRECTION_BASE + (irq * 2) + 1;

    struct ioapic_entry entry = {};

    entry.vector = vector;
    entry.delivery_mode = delivery_mode;
    entry.destination_mode = !physical_destination; // 0 for Physical, 1 for Logical
    entry.delivery_status = delivery_status;        // 1 for In-process
    entry.pin_polarity = pin_polarity;              // 1 for Active Low, 0 for Active High
    entry.trigger_mode = level_triggered;           // 1 for Level, 0 for Edge
    entry.mask = mask_irq;                          // 1 to mask the IRQ, 0 to unmask

    if (physical_destination)
    {
        entry.destination = (uint8_t)(destination & 0x0F);
    }
    else
    {
        entry.destination = (uint8_t)(destination & 0xFF);
    }

    uint64_t redirection = 0;

    redirection |= (uint64_t)(entry.vector);
    redirection |= (uint64_t)(entry.delivery_mode) << 8;
    redirection |= (uint64_t)(entry.destination_mode) << 11;
    redirection |= (uint64_t)(entry.delivery_status) << 12;
    redirection |= (uint64_t)(entry.pin_polarity) << 13;
    redirection |= (uint64_t)(entry.remote_irr) << 14;
    redirection |= (uint64_t)(entry.trigger_mode) << 15;
    redirection |= (uint64_t)(entry.mask) << 16;
    redirection |= (uint64_t)(entry.destination) << 56;

    ioapic_write(ioapic_base, high_index, (uint32_t)(redirection >> 32));
    ioapic_write(ioapic_base, low_index, (uint32_t)(redirection & 0xFFFFFFFF));
}

uint64_t ioapic_get_redirection(uintptr_t ioapic_base, uint8_t irq)
{
    uint32_t low_index = IOAPIC_REGISTER_REDIRECTION_BASE + (irq * 2);
    uint32_t high_index = IOAPIC_REGISTER_REDIRECTION_BASE + (irq * 2) + 1;

    uint32_t low = ioapic_read(ioapic_base, low_index);
    uint32_t high = ioapic_read(ioapic_base, high_index);

    return ((uint64_t)(high) << 32) | low;
}

uint8_t get_ioapic_max_redirections(uintptr_t ioapic_base)
{
    uint32_t version = ioapic_read(ioapic_base, 0x01);
    return ((version >> 16) & 0xFF) + 1;
}

int get_gsi_out_of_irq(int irq) {
    struct interrupt_source_override_linked_list_entry* entry = interrupt_source_override_linked_list_head;
    while (entry) {
        if (entry->interrupt_source_override->source == irq) {
            return entry->interrupt_source_override->global_system_interrupt;
        }
        entry = entry->next;
    }
    return -1;
}

void init_lapic() {
    // aight cuh read the msr
    uint64_t lapic_address = rdmsr(IA32_APIC_BASE);

    lapic_address &= ~(1 << 11); // apic global enable flag, disable it, we need address
    lapic_address &= ~(1 << 8); // bsp flag, disable it, we need address cuh

    lapic_address += get_hhdm_slide(); // make it virtual cuh

    // mm spuriouse
    apic_write(lapic_address, APIC_REGISTER_SPURIOUS,
               apic_read(lapic_address, APIC_REGISTER_SPURIOUS) | 0x100);
}

volatile float pit_sleep_for_ms = 0;
volatile uint32_t ticks_in_time = 0;

volatile uint32_t lapic_ticks_per_ms = 0;
volatile float ms_elapsed = 0;

volatile float tick = 0.062f;

void(*secondary_lapic_timer_handler)(struct interrupt_frame*) = NULL;

void set_lapic_secondary_timer_handler(void(*handler)(struct interrupt_frame*)) {
    secondary_lapic_timer_handler = handler;
}

uint32_t idt_vector_to_irq(uint32_t vector) {
    int cpu_id = get_lapic_id();
    struct cpu_local_info *linfo = &cpu_local_info[cpu_id];
    struct cpu_irq_info_entry *current = linfo->head;

    while (current != NULL) {
        if (current->vector == vector) {
            return current->gsi;
        }
        current = current->next;
    }
    return -1;
}

void lapic_timer_handler(struct interrupt_frame *frame) {
    ms_elapsed += tick;

    if (secondary_lapic_timer_handler) {
        secondary_lapic_timer_handler(frame);
    }
}

void prepare_pit_sleep(float sleep_time) {
    pit_sleep_for_ms = sleep_time;
}

void pit_timer_handler(struct interrupt_frame* frame) {
    pit_sleep_for_ms -= tick;
}

void pit_sleep(float ms) {
    prepare_pit_sleep(ms);
    while (pit_sleep_for_ms > 0) {
        asm volatile ("pause");
    }
}

void pit_set_frequency(uint32_t frequency) {
    uint16_t divisor = (uint16_t)(PIT_BASE_HZ / frequency);

    debug_print("Using PIT divisor: %hd\n", divisor);

    outb(PIT_COMMAND_PORT, 0x36);  // Channel 0, lobyte/hibyte, mode 2

    // send divisor
    outb(PIT_CHANNEL0_PORT, (uint8_t) (divisor & 0xFF)); // low byte
    outb(PIT_CHANNEL0_PORT, (uint8_t) ((divisor >> 8) & 0xFF)); // high byte
}

// setup and calibrate LAPIC timer
void time_up_lapic() {
    idt_set_handler(255, lapic_timer_handler);

    uint32_t lapic_divisor = 3;
    uint32_t calibration_ms = 100;

    // Mask LAPIC timer
    apic_write(get_lapic_address(), APIC_REGISTER_LVT_TIMER, APIC_LVT_INT_MASKED);

    // Set LAPIC timer to max count
    apic_write(get_lapic_address(), APIC_REGISTER_TIMER_DIVIDE, lapic_divisor);
    apic_write(get_lapic_address(), APIC_REGISTER_TIMER_INITIAL, 0xFFFFFFFF);

    // Set up PIT for calibration_ms ms
    pit_set_frequency(16130);
    register_interrupt_handler(0, true, pit_timer_handler);

    asm volatile("sti");
    pit_sleep((float)calibration_ms);

    // Read LAPIC timer current count
    uint32_t elapsed = 0xFFFFFFFF - apic_read(get_lapic_address(), APIC_REGISTER_TIMER_CURRENT);
    lapic_ticks_per_ms = elapsed / calibration_ms;
    if (lapic_ticks_per_ms == 0) lapic_ticks_per_ms = 1; // avoid div by zero

    debug_print("LAPIC ticks per ms: %u\n", lapic_ticks_per_ms);

    // mask timer and cleanup
    apic_write(get_lapic_address(), APIC_REGISTER_LVT_TIMER,
               APIC_LVT_INT_MASKED);
    unregister_interrupt_handler(0, false);

    debug_log("Setting up periodic LAPIC timer\n");

    // Set LAPIC timer to periodic mode, 1ms interval
    apic_write(get_lapic_address(), APIC_REGISTER_LVT_TIMER, 255 | APIC_TIMER_MODE_PERIODIC);
    apic_write(get_lapic_address(), APIC_REGISTER_TIMER_DIVIDE, lapic_divisor);
    apic_write(get_lapic_address(), APIC_REGISTER_TIMER_INITIAL, lapic_ticks_per_ms);

    ms_elapsed = tick;
}

void apic_init() {
    void* table = acpi_get_table("APIC");
    debug_print("APIC table address: %p\n", table);

    // get all the apic entries cuh

    struct apic* apic_table = table;
    uint64_t lapic_address = (uint64_t) apic_table->local_controller_address;

    // parse entries
    uint64_t table_entries_size = apic_table->sdt.length - sizeof(struct sdt) - sizeof(uint32_t) * 2;
    uint64_t read = 0;
    while (table_entries_size > read) {
        struct apic_entry* entry = (struct apic_entry*)((uintptr_t)apic_table + sizeof(struct apic) + read);

        switch (entry->type) {
            case 0: // LAPIC
            {
                struct local_apic_entry* local_apic_entry = (struct local_apic_entry*)entry;

                debug_print("Local APIC: \n");
                debug_print("Local APIC ID: %d\n", local_apic_entry->apic_id);
                debug_print("Flags: %x\n", local_apic_entry->flags);
                debug_print("Processor ID: %d\n", local_apic_entry->processor_id);

                if (!lapic_linked_list_head) {
                    struct lapic_linked_list_entry* lapic_ll_ent = malloc(sizeof(struct lapic_linked_list_entry));
                    lapic_ll_ent->next = NULL;
                    lapic_ll_ent->local_apic = local_apic_entry;
                    lapic_linked_list_head = lapic_ll_ent;
                } else {
                    struct lapic_linked_list_entry* lapic_ll_ent = malloc(sizeof(struct lapic_linked_list_entry));
                    lapic_ll_ent->next = lapic_linked_list_head;
                    lapic_ll_ent->local_apic = local_apic_entry;
                    lapic_linked_list_head = lapic_ll_ent;
                }

                break;
            }
            case 1: // IOAPIC
            {
                struct io_apic_entry* io_apic_entry = (struct io_apic_entry*)entry;

                debug_print("IO APIC: \n");
                debug_print("IO APIC ID: %d\n", io_apic_entry->io_apic_id);
                debug_print("IO APIC Address: %04x\n", io_apic_entry->io_apic_address);
                debug_print("IO APIC Global System Interrupt Base: %d\n", io_apic_entry->global_system_interrupt_base);

                if (!ioapic_linked_list_head) {
                    struct ioapic_linked_list_entry* ioapic_ll_ent = malloc(sizeof(struct ioapic_linked_list_entry));
                    ioapic_ll_ent->next = NULL;
                    ioapic_ll_ent->io_apic = io_apic_entry;
                    ioapic_linked_list_head = ioapic_ll_ent;
                } else {
                    struct ioapic_linked_list_entry* entry = malloc(sizeof(struct ioapic_linked_list_entry));
                    entry->next = ioapic_linked_list_head;
                    entry->io_apic = io_apic_entry;
                    ioapic_linked_list_head = entry;
                }

                break;
            }
            case 2: // Interrupt Source Override
            {
                struct interrupt_source_override_entry* interrupt_source_override_entry = (struct interrupt_source_override_entry*)entry;

                debug_print("Interrupt Source Override: \n");
                debug_print("Bus: %d\n", interrupt_source_override_entry->bus);
                debug_print("Source: %hhd\n", interrupt_source_override_entry->source);
                debug_print("GSI: %d\n", interrupt_source_override_entry->global_system_interrupt);
                debug_print("Flags: %d\n", interrupt_source_override_entry->flags);

                if (!interrupt_source_override_linked_list_head) {
                    struct interrupt_source_override_linked_list_entry* iso_ll_ent = malloc(sizeof(struct interrupt_source_override_linked_list_entry));
                    iso_ll_ent->next = NULL;
                    iso_ll_ent->interrupt_source_override = interrupt_source_override_entry;
                    interrupt_source_override_linked_list_head = iso_ll_ent;
                } else {
                    struct interrupt_source_override_linked_list_entry* iso_ll_ent = malloc(sizeof(struct interrupt_source_override_linked_list_entry));
                    iso_ll_ent->next = interrupt_source_override_linked_list_head;
                    iso_ll_ent->interrupt_source_override = interrupt_source_override_entry;
                    interrupt_source_override_linked_list_head = iso_ll_ent;
                }

                break;
            }
            default: debug_print("Unknown APIC entry type: %d\n", entry->type);
        }

        read += entry->length;
    }

    // now we gotta enable our lapic cuh
    // get the msr for lapic cuh
    init_lapic();

    // we gotta set first 32 / 8 (4) bytes of vectors
    for (int i = 0; MAX_CPUS > i; i++) {
        struct cpu_local_info* linfo = &cpu_local_info[i];
        struct cpu_vectors* vecs = &linfo->vectors;

        for (int j = 0; 32 / (sizeof(uint8_t) * 8) > j; j++) {
            vecs->vectors[j] = 0xFF;
        }
    }

    // time up the lapic timer
    time_up_lapic();
}

uintptr_t get_ioapic_base_address(int this_gsi) {
    // now we gotta find *the* ioapic

   struct ioapic_linked_list_entry* entry = ioapic_linked_list_head;
   while (entry) {
       debug_print("Checking IOAPIC %p for GSI %d\n", entry->io_apic, this_gsi);
       if (entry->io_apic->global_system_interrupt_base <= this_gsi && this_gsi < entry->io_apic->global_system_interrupt_base + get_ioapic_max_redirections(entry->io_apic->io_apic_address + get_hhdm_slide())) {
           return entry->io_apic->io_apic_address + get_hhdm_slide();
       }
       entry = entry->next;
   }

    return -1;
}

int get_ioapic_gsi_base(uintptr_t ioapic_base) {
    struct ioapic_linked_list_entry *entry = ioapic_linked_list_head;
    while (entry) {
        if (entry->io_apic->io_apic_address + get_hhdm_slide() == ioapic_base) {
            return entry->io_apic->global_system_interrupt_base;
        }
        entry = entry->next;
    }
    return -1;
}

void register_interrupt_handler(int vec, bool is_irq, void(*handler)(struct interrupt_frame*)) {
    int this_gsi;

    if (is_irq) {
        this_gsi = get_gsi_out_of_irq(vec);
        if (this_gsi == -1) {
            this_gsi = vec; // fallback
        }
    } else {
        this_gsi = vec;
    }

    int cpu_id = get_lapic_id();
    struct cpu_local_info* linfo = &cpu_local_info[cpu_id];
    struct cpu_vectors* vecs_structure = &linfo->vectors;
    uint8_t* vecs = vecs_structure->vectors;

    int vector = -1;

    for (int i = 32; i < 256; ++i) {
        int byte_index = i / 8;
        int bit_index  = i % 8;

        if (!(vecs[byte_index] & (1 << bit_index))) {
            vector = i;
            vecs[byte_index] |= (1 << bit_index);
            break;
        }
    }

    if (vector == -1) {
        debug_print("No free IDT vector found!\n");
        return;
    }

    debug_print("Registering interrupt handler for IDT vector %d (GSI %d) on CPU %d\n", vector, this_gsi, cpu_id);

    struct cpu_irq_info_entry* entry = malloc(sizeof(struct cpu_irq_info_entry));
    entry->gsi = this_gsi;
    entry->vector = vector;
    entry->handler = handler;
    entry->next = linfo->head;
    linfo->head = entry;

    // Set the handler in the IDT
    idt_set_handler(vector, handler);

    // Route GSI to LAPIC vector
    uintptr_t ioapic_base = get_ioapic_base_address(this_gsi);
    ioapic_set_redirection(ioapic_base, this_gsi - get_ioapic_gsi_base(ioapic_base), cpu_id, vector,
                           0b000, false, true, false, false, false);
}

void unregister_interrupt_handler(int irq, bool is_gsi) {
    int this_gsi;
    if (!is_gsi) {
        this_gsi = get_gsi_out_of_irq(irq);
        if (this_gsi == -1) {
            this_gsi = irq;
        }
    } else {
        this_gsi = irq;
    }

    int cpu_id = get_lapic_id();
    struct cpu_local_info *linfo = &cpu_local_info[cpu_id];
    struct cpu_irq_info_entry *current = linfo->head;
    struct cpu_irq_info_entry *prev = NULL;

    while (current != NULL) {
        if (current->gsi == this_gsi) {
            debug_print("Unregistering interrupt handler for GSI %d on CPU %d\n", this_gsi, cpu_id);
            struct cpu_vectors *vecs = &linfo->vectors;

            int vector = current->vector;

            // Unset IDT handler
            idt_set_handler(vector, NULL);

            // Clear vector bit
            int byte_index = vector / 8;
            int bit_index = vector % 8;
            vecs->vectors[byte_index] &= ~(1 << bit_index);

            // Mask in IOAPIC
            uintptr_t ioapic_base = get_ioapic_base_address(this_gsi);
            ioapic_set_redirection(ioapic_base, this_gsi - get_ioapic_gsi_base(ioapic_base),
                                   cpu_id, vector, 0, false, true, false, false,
                                   true);

            // unlink entry
            if (prev == NULL) {
                linfo->head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        debug_print("No interrupt handler found for GSI %d on CPU %d\n", this_gsi, cpu_id);
    }
}

void lapic_send_eoi() {
    apic_write(get_lapic_address(), APIC_REGISTER_EOI, 0);
}

uint64_t get_time_since_boot() {
    return (uint64_t)(ms_elapsed * 1000000.0f);
}
