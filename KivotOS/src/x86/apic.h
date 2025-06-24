//
// Created by neko on 6/8/25.
//

#ifndef APIC_H
#define APIC_H

#include <stdbool.h>
#include <stdint.h>
#include <x86/idt.h>

#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40
#define PIT_FREQUENCY 1000
#define PIT_BASE_HZ 1193182

#define MAX_CPUS 512

#define IA32_APIC_BASE 0x1B
#define APIC_REGISTER_ID 0x020           // Local APIC ID Register
#define APIC_REGISTER_VERSION 0x030      // Local APIC Version Register
#define APIC_REGISTER_TASK_PRIORITY 0x080 // Task Priority Register
#define APIC_REGISTER_EOI 0x0B0          // End of Interrupt Register
#define APIC_REGISTER_SPURIOUS 0x0F0     // Spurious Interrupt Vector Register
#define APIC_REGISTER_ICR_LOW 0x300      // Interrupt Command Register (low)
#define APIC_REGISTER_ICR_HIGH 0x310     // Interrupt Command Register (high)
#define APIC_REGISTER_LVT_TIMER 0x320    // Local Vector Table (LVT) Timer Register
#define APIC_REGISTER_LVT_THERMAL 0x330  // LVT Thermal Sensor Register
#define APIC_REGISTER_LVT_PERF 0x340     // LVT Performance Counter Register
#define APIC_REGISTER_LVT_LINT0 0x350    // LVT LINT0 Register
#define APIC_REGISTER_LVT_LINT1 0x360    // LVT LINT1 Register
#define APIC_REGISTER_LVT_ERROR 0x370    // LVT Error Register
#define APIC_REGISTER_TIMER_INITIAL 0x380 // Timer Initial Count Register
#define APIC_REGISTER_TIMER_CURRENT 0x390 // Timer Current Count Register
#define APIC_REGISTER_TIMER_DIVIDE 0x3E0 // Timer Divide Configuration Register

#define IOAPIC_REGISTER_IOAPICID 0x00         // IO APIC ID
#define IOAPIC_REGISTER_IOAPICVER 0x01        // IO APIC Version
#define IOAPIC_REGISTER_IOAPICARB 0x02        // IO APIC Arbitration ID
#define IOAPIC_REGISTER_REDIRECTION_BASE 0x10 // Redirection Table Base

#define APIC_LVT_INT_MASKED (1 << 16)
#define APIC_TIMER_MODE_PERIODIC (1 << 17)

void apic_init();

uint32_t get_lapic_id();
void set_lapic_secondary_timer_handler(void(*handler)(struct interrupt_frame*));
uint32_t idt_vector_to_irq(uint32_t vector);
void register_interrupt_handler(int vec, bool is_irq, void (*handler)(struct interrupt_frame*));
void unregister_interrupt_handler(int irq, bool is_gsi);
void lapic_send_eoi();
uint64_t get_time_since_boot();

#endif //APIC_H
