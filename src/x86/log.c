//
// Created by neko on 6/7/25.
//

#define NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 1

#include "log.h"

#include <x86/io.h>
#include <x86/apic.h>
#include <stdarg.h>
#include <rt/nanoprintf.h>

void debug_log(const char *s) {
    char buffer[1024];
    npf_snprintf(buffer, sizeof(buffer), "[%.6f] %s",
                 (float) get_time_since_boot() / 1000000000.0f, s);
    const char *p = buffer;
    while (*p) {
        outb(0xe9, *p);
        p++;
    }
}

void debug_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char temp[1024] = {0};
    const int written = npf_vsnprintf(temp, 1024, fmt, args);
    temp[written] = '\0';

    char buffer[1024];
    npf_snprintf(buffer, sizeof(buffer), "[%.6f] %s",
                 (float) get_time_since_boot() / 1000000000.0f, temp);
    const char *p = buffer;
    while (*p) {
        outb(0xe9, *p);
        p++;
    }

    va_end(args);
}
