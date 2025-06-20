//
// Created by neko on 6/17/25.
//

#include <stdlib.h>
#include <mem/virtual.h>
#include <rt/mutex.h>
#include <sched/sched.h>
#include <uacpi/kernel_api.h>
#include <x86/apic.h>
#include <x86/log.h>

#include "acpi.h"
#include "uacpi_io.h"

#define UNIMPLEMENTED() unimplemented(__FILE__, __LINE__, __FUNCTION__);

__attribute__((used)) uacpi_status unimplemented(const char* file, int line, const char* func) {
    debug_print("Unimplemented function %s in %s:%d\n", func, file, line);
    asm volatile ("cli; hlt");
    return UACPI_STATUS_UNIMPLEMENTED;
}

__attribute__((used)) uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    *out_rsdp_address = (uint64_t)acpi_get_physical_rsdp_address();
    return UACPI_STATUS_OK;
}

__attribute__((used)) void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    return (void*)(addr + get_hhdm_slide());
}

__attribute__((used)) void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    return;
}

#ifndef UACPI_FORMATTED_LOGGING
__attribute__((used)) void uacpi_kernel_log(uacpi_log_level log_level, const uacpi_char* msg) {
    const char* prefix = NULL;
    switch (log_level) {
        case UACPI_LOG_DEBUG: prefix = "[DEBUG]"; break;
        case UACPI_LOG_TRACE: prefix = "[TRACE]"; break;
        case UACPI_LOG_INFO: prefix = "[INFO]"; break;
        case UACPI_LOG_WARN: prefix = "[WARN]"; break;
        case UACPI_LOG_ERROR: prefix = "[ERROR]"; break;
        default: prefix = "[???]"; break;
    }

    debug_print("%s %s", prefix, msg);
}
#else
UACPI_PRINTF_DECL(2, 3)
void uacpi_kernel_log(uacpi_log_level, const uacpi_char*, ...) {
    UNIMPLEMENTED()
}
__attribute__((used)) void uacpi_kernel_vlog(uacpi_log_level, const uacpi_char*, uacpi_va_list) {
    UNIMPLEMENTED()
}
#endif

#ifdef UACPI_KERNEL_INITIALIZATION
/*
 * This API is invoked for each initialization level so that appropriate parts
 * of the host kernel and/or glue code can be initialized at different stages.
 *
 * uACPI API that triggers calls to uacpi_kernel_initialize and the respective
 * 'current_init_lvl' passed to the hook at that stage:
 * 1. uacpi_initialize() -> UACPI_INIT_LEVEL_EARLY
 * 2. uacpi_namespace_load() -> UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED
 * 3. (start of) uacpi_namespace_initialize() -> UACPI_INIT_LEVEL_NAMESPACE_LOADED
 * 4. (end of) uacpi_namespace_initialize() -> UACPI_INIT_LEVEL_NAMESPACE_INITIALIZED
 */
__attribute__((used)) uacpi_status uacpi_kernel_initialize(uacpi_init_level current_init_lvl) {
    return UNIMPLEMENTED()
}
__attribute__((used)) void uacpi_kernel_deinitialize(void) {
    UNIMPLEMENTED()
}
#endif

__attribute__((used)) uacpi_status uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
) {
    return UACPI_STATUS_UNIMPLEMENTED;
    return UNIMPLEMENTED()
}

__attribute__((used)) void uacpi_kernel_pci_device_close(uacpi_handle) {
    UNIMPLEMENTED()
}

__attribute__((used)) uacpi_status uacpi_kernel_pci_read8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 *value
) {
    return UNIMPLEMENTED()
}
__attribute__((used)) uacpi_status uacpi_kernel_pci_read16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 *value
) {
    return UNIMPLEMENTED()
}
__attribute__((used)) uacpi_status uacpi_kernel_pci_read32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 *value
) {
    return UNIMPLEMENTED()
}

__attribute__((used)) uacpi_status uacpi_kernel_pci_write8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 value
) {
    return UNIMPLEMENTED()
}

__attribute__((used)) uacpi_status uacpi_kernel_pci_write16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 value
) {
    return UNIMPLEMENTED()
}
__attribute__((used)) uacpi_status uacpi_kernel_pci_write32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 value
) {
    return UNIMPLEMENTED()
}

__attribute__((used)) uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
) {
    *out_handle = create_io_region(base, len);
    return UACPI_STATUS_OK;
}
__attribute__((used)) void uacpi_kernel_io_unmap(uacpi_handle handle) {
    free_io_region(handle);
}

__attribute__((used)) uacpi_status uacpi_kernel_io_read8(
    uacpi_handle h, uacpi_size offset, uacpi_u8 *out_value
) {
    *out_value = ior_read8(h, offset);
    return UACPI_STATUS_OK;
}
__attribute__((used)) uacpi_status uacpi_kernel_io_read16(
    uacpi_handle h, uacpi_size offset, uacpi_u16 *out_value
) {
    *out_value = ior_read16(h, offset);
    return UACPI_STATUS_OK;
}

__attribute__((used)) uacpi_status uacpi_kernel_io_read32(
    uacpi_handle h, uacpi_size offset, uacpi_u32 *out_value
) {
    *out_value = ior_read32(h, offset);
    return UACPI_STATUS_OK;
}

__attribute__((used)) uacpi_status uacpi_kernel_io_write8(
    uacpi_handle h, uacpi_size offset, uacpi_u8 in_value
) {
    ior_write8(h, offset, in_value);
    return UACPI_STATUS_OK;
}

__attribute__((used)) uacpi_status uacpi_kernel_io_write16(
    uacpi_handle h, uacpi_size offset, uacpi_u16 in_value
) {
    ior_write16(h, offset, in_value);
    return UACPI_STATUS_OK;
}

__attribute__((used)) uacpi_status uacpi_kernel_io_write32(
    uacpi_handle h, uacpi_size offset, uacpi_u32 in_value
) {
    ior_write32(h, offset, in_value);
    return UACPI_STATUS_OK;
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return malloc(size);
}

#ifdef UACPI_NATIVE_ALLOC_ZEROED
/*
 * Allocate a block of memory of 'size' bytes.
 * The returned memory block is expected to be zero-filled.
 */
void *uacpi_kernel_alloc_zeroed(uacpi_size size) {
    UNIMPLEMENTED()
}
#endif

#ifndef UACPI_SIZED_FREES
void uacpi_kernel_free(void *mem) {
    free(mem);
}
#else
void uacpi_kernel_free(void *mem, uacpi_size size_hint) {
    UNIMPLEMENTED()
}
#endif

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return get_time_since_boot();
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    UNIMPLEMENTED()
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    UNIMPLEMENTED()
}

uacpi_handle uacpi_kernel_create_mutex(void) {
    struct mutex* mtx = malloc(sizeof(struct mutex));
    mtx->locked = 0;
    return mtx;
}
void uacpi_kernel_free_mutex(uacpi_handle mtx) {
    free(mtx);
}

uacpi_handle uacpi_kernel_create_event(void) {
    struct mutex *mtx = malloc(sizeof(struct mutex));
    mtx->locked = 0;
    return mtx;
}

void uacpi_kernel_free_event(uacpi_handle h) {
    free(h);
}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return get_current_process()->tid;
}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle h, uacpi_u16 timeout) {
    lock_mutex(h);
    return UACPI_STATUS_OK;
}
void uacpi_kernel_release_mutex(uacpi_handle h) {
    unlock_mutex(h);
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16) {
    UNIMPLEMENTED()
}

void uacpi_kernel_signal_event(uacpi_handle) {
    UNIMPLEMENTED()
}

void uacpi_kernel_reset_event(uacpi_handle) {
    UNIMPLEMENTED()
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*) {
    UNIMPLEMENTED()
}

struct uacpi_irq_to_handler {
    uacpi_u32 irq;
    uacpi_interrupt_handler handler;
    uacpi_handle ctx;
    struct uacpi_irq_to_handler* next;
};

struct uacpi_irq_to_handler* irq_to_handler_list = NULL;

void uacpi_interrupt_handler_commmon(struct interrupt_frame* frame) {
    uint32_t irq = idt_vector_to_irq(frame->interrupt_number);

    if (irq == -1) {
        debug_print("Invalid IRQ in %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
    }

    if (!irq_to_handler_list)
        return;

    struct uacpi_irq_to_handler *current = irq_to_handler_list;
    while (current) {
        if (current->irq == irq)
            current->handler(current->ctx);
        current = current->next;
    }

    lapic_send_eoi();
}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle) {
    struct uacpi_irq_to_handler *new_handler = malloc(
        sizeof(struct uacpi_irq_to_handler));
    if (!new_handler)
        return UACPI_STATUS_OUT_OF_MEMORY;

    new_handler->irq = irq;
    new_handler->handler = handler;
    new_handler->ctx = ctx;
    new_handler->next = irq_to_handler_list;
    irq_to_handler_list = new_handler;

    register_interrupt_handler(irq, true, uacpi_interrupt_handler_commmon);

    *out_irq_handle = new_handler;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler handler, uacpi_handle irq_handle
) {
    if (!irq_handle)
        return UACPI_STATUS_INVALID_ARGUMENT;

    struct uacpi_irq_to_handler *to_remove = (struct uacpi_irq_to_handler *)
            irq_handle;

    if (irq_to_handler_list == to_remove) {
        irq_to_handler_list = to_remove->next;
    } else {
        struct uacpi_irq_to_handler *current = irq_to_handler_list;
        while (current && current->next != to_remove) {
            current = current->next;
        }
        if (current) {
            current->next = to_remove->next;
        }
    }

    unregister_interrupt_handler(to_remove->irq, true);
    free(to_remove);
    return UACPI_STATUS_OK;
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
    return uacpi_kernel_create_mutex();
}
void uacpi_kernel_free_spinlock(uacpi_handle spl) {
    uacpi_kernel_free_mutex(spl);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle spl) {
    lock_mutex(spl);
    return 0;
}
void uacpi_kernel_unlock_spinlock(uacpi_handle spl, uacpi_cpu_flags) {
    unlock_mutex(spl);
}

uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type, uacpi_work_handler, uacpi_handle ctx
) {
    UNIMPLEMENTED()
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    UNIMPLEMENTED()
}