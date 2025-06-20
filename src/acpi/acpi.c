//
// Created by neko on 6/8/25.
//

#include "acpi.h"

#include <stdbool.h>
#include <string.h>
#include <mem/virtual.h>
#include <uacpi/event.h>
#include <uacpi/status.h>
#include <uacpi/uacpi.h>
#include <x86/log.h>


volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = LIMINE_API_REVISION,
    .response = NULL
};

struct rsdp* rsdp = NULL;
struct rsdt* rsdt = NULL;
struct xsdt* xsdt = NULL;
bool using_xsdt = false;

void acpi_discover_xsdt_tables() {
    debug_print("Discovering XSDT tables.\n");

    size_t tables_offset = sizeof(struct xsdt);

    uint64_t* tables_address = (uint64_t*)((uintptr_t)xsdt + tables_offset);
    size_t entry_count = (xsdt->sdt.length - sizeof(struct sdt)) / sizeof(uint64_t);

    for (size_t i = 0; i < entry_count; i++) {
        uint64_t table_address = tables_address[i];
        void* virtual_table_address = (void*)(table_address + get_hhdm_slide());
        struct sdt* table_sdt = virtual_table_address;

        debug_print("Table %lu: %lx\n", i, table_address);

        char signature[5];
        memcpy(signature, table_sdt->signature, 4);
        signature[4] = '\0';

        debug_print("\tTable signature: %s\n", signature);
    }
}

void acpi_discover_rsdt_tables() {
    debug_print("Discovering RSDT tables.\n");
    asm volatile ("cli; hlt");
}

void acpi_discover_tables() {
    if (using_xsdt) {
        acpi_discover_xsdt_tables();
    } else {
        acpi_discover_rsdt_tables();
    }
}

void acpi_init() {
    struct limine_rsdp_response* response = rsdp_request.response;

    if (response == NULL) {
        debug_print("No ACPI available.\n");
        return;
    }

    rsdp = (struct rsdp*) response->address;

    if (rsdp->revision >= 2) {
        // using xsdt
        using_xsdt = true;
        xsdt = (struct xsdt*)(rsdp->xsdt_address + get_hhdm_slide());
    } else {
        // using rsdt
        rsdt = (struct rsdt*)((uint64_t)rsdp->rsdt_address + get_hhdm_slide());
    }

    acpi_discover_tables();
}

void * acpi_get_table_by_xsdt(char signature[4]) {
    if (using_xsdt) {
        size_t tables_offset = sizeof(struct xsdt);

        uint64_t* tables_address = (uint64_t*)((uintptr_t)xsdt + tables_offset);
        size_t entry_count = (xsdt->sdt.length - sizeof(struct sdt)) / sizeof(uint64_t);

        for (size_t i = 0; i < entry_count; i++) {
            uint64_t table_address = tables_address[i];
            void* virtual_table_address = (void*)(table_address + get_hhdm_slide());
            struct sdt* table_sdt = virtual_table_address;

            char table_signature[5];
            memcpy(table_signature, table_sdt->signature, 4);
            table_signature[4] = '\0';

            if (strcmp(table_signature, signature) == 0) {
                return virtual_table_address;
            }
        }
        return NULL;
    }
}

void * acpi_get_table_by_rsdt(char signature[4]) {
    return NULL; // unimplemented the rsdt one.
}

void * acpi_get_table(char signature[4]) {
    if (using_xsdt) {
        return acpi_get_table_by_xsdt(signature);
    } else {
        return acpi_get_table_by_rsdt(signature);
    }
}

void * acpi_get_physical_rsdp_address() {
    debug_print("RSDP address: %016lX\n", rsdp_request.response->address);
    return (void*) (rsdp_request.response->address - get_hhdm_slide());
}

void acpi_further_init() {
    /*
     * Start with this as the first step of the initialization. This loads all
     * tables, brings the event subsystem online, and enters ACPI mode. We pass
     * in 0 as the flags as we don't want to override any default behavior for now.
     */
    uacpi_status ret = uacpi_initialize(0);
    if (uacpi_unlikely_error(ret)) {
        debug_print("uacpi_initialize error: %s", uacpi_status_to_string(ret));
        asm volatile ("cli; hlt");
    }

    /*
     * Load the AML namespace. This feeds DSDT and all SSDTs to the interpreter
     * for execution.
     */
    ret = uacpi_namespace_load();
    if (uacpi_unlikely_error(ret)) {
        debug_print("uacpi_namespace_load error: %s", uacpi_status_to_string(ret));
        asm volatile ("cli; hlt");
    }

    /*
     * Initialize the namespace. This calls all necessary _STA/_INI AML methods,
     * as well as _REG for registered operation region handlers.
     */
    ret = uacpi_namespace_initialize();
    if (uacpi_unlikely_error(ret)) {
        debug_print("uacpi_namespace_initialize error: %s", uacpi_status_to_string(ret));
        asm volatile ("cli; hlt");
    }

    /*
     * Tell uACPI that we have marked all GPEs we wanted for wake (even though we haven't
     * actually marked any, as we have no power management support right now). This is
     * needed to let uACPI enable all unmarked GPEs that have a corresponding AML handler.
     * These handlers are used by the firmware to dynamically execute AML code at runtime
     * to e.g. react to thermal events or device hotplug.
     */
    ret = uacpi_finalize_gpe_initialization();
    if (uacpi_unlikely_error(ret)) {
        debug_print("uACPI GPE initialization error: %s", uacpi_status_to_string(ret));
        asm volatile ("cli; hlt");
    }
}
