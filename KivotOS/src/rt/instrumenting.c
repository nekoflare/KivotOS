#include "instrumenting.h"

#include <x86/log.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <x86/interrupts_helpers.h>

#include "kallsyms.h"

__attribute__((no_instrument_function))
static inline uintptr_t get_current_stack_pointer(void) {
    uintptr_t rsp;
    asm volatile("mov %%rsp, %0" : "=r"(rsp));
    return rsp;
}

#define TRACE_BUFFER_SIZE 50

static const char* trace[TRACE_BUFFER_SIZE] = {0};
static size_t trace_index = 0;

static bool in_instr_enter = false;
static bool tracing_enabled = true;

extern char stack_bottom[];
extern char stack_top[];

extern uint64_t heap_base;
extern uint64_t current_heap_size;

uint64_t max_stack_usage = 0;

__attribute__((no_instrument_function))
void add_to_trace(const char* str) {
    if (trace_index >= TRACE_BUFFER_SIZE) {
        memmove(&trace[0], &trace[1],
                sizeof(const char *) * (TRACE_BUFFER_SIZE - 1));
        trace[TRACE_BUFFER_SIZE - 1] = str;
    } else {
        trace[trace_index++] = str;
    }
}

__attribute__((no_instrument_function))
void __cyg_profile_func_enter(void *this_fn, void *call_site) {
    if (!tracing_enabled || in_instr_enter) {
        return;
    }
    in_instr_enter = true;

    const char* fname = kallsyms_nearest((uintptr_t) this_fn);
    add_to_trace(fname);

    uint64_t rsp = get_current_stack_pointer();

    // Only do stack usage analysis if within expected bounds
    if (rsp >= (uintptr_t)&stack_top && rsp <= (uintptr_t)&stack_bottom) {
        uint64_t stack_usage = (uintptr_t)&stack_bottom - rsp;

        if (stack_usage > max_stack_usage) {
            debug_print("Max stack usage changed. Previous: %lu. Current: %lu RSP: %016lX Stack top: %016lX Stack bottom: %016lX\n",
                        max_stack_usage, stack_usage, rsp, (uintptr_t)&stack_top, (uintptr_t)&stack_bottom);
            max_stack_usage = stack_usage;
        }
    }

    in_instr_enter = false;
}

__attribute__((no_instrument_function))
void __cyg_profile_func_exit(void *this_fn, void *call_site) {
}

void pause_logging_calls() {
    tracing_enabled = false;
}

void resume_logging_calls() {
    tracing_enabled = true;
}

__attribute__((no_instrument_function))
void print_call_log() {
    disable_printing_time(); // disable timestamps during postmortem

    in_instr_enter = true;
    tracing_enabled = false;

    debug_print("\n=== Postmortem Call Trace (Last %d Entries) ===\n", TRACE_BUFFER_SIZE);

    for (size_t i = 0; i < TRACE_BUFFER_SIZE; i++) {
        debug_print("- %s\n", trace[i]);
    }

    in_instr_enter = false;
}


__attribute__((no_instrument_function))
void print_stack_and_heap_info(struct interrupt_frame* frame) {
    uintptr_t rsp = frame->orig_rsp;
    uintptr_t stack_top_addr = (uintptr_t)&stack_bottom;

    debug_print("RSP = %016lX Stack top: %016lX\n", rsp, stack_top_addr);

    size_t stack_used = stack_top_addr > rsp ? stack_top_addr - rsp : 0;

    debug_print("\n=== Usages ===\n");
    debug_print("Stack usage: %zu bytes\n", stack_used);
    debug_print("Max stack usage: %zu bytes\n", max_stack_usage);
    debug_print("Heap usage: %lu bytes (base: %p)\n", current_heap_size, (void*)heap_base);
}