//
// Created by neko on 6/8/25.
//

#include <stdio.h>
#include <stdarg.h>
#include <x86/log.h>

#include "nanoprintf.h"

FILE * stderr = (FILE*)(1);

int fprintf (FILE *__restrict __stream,
                    const char *__restrict __format, ...) {
    (void)__stream;

    va_list ap;
    va_start(ap, __format);

    char buffer [1024];
    const int written = npf_vsnprintf(buffer, 1024, __format, ap);
    buffer[1023] = 0; // null terminate

    debug_log(buffer);

    va_end(ap);
    return written;
}

int snprintf(char *__restrict __s, size_t __maxlen,
             const char *__restrict __format, ...) {
    va_list ap;
    va_start(ap, __format);
    const int written = npf_vsnprintf(__s, __maxlen, __format, ap);
    va_end(ap);
    return written;
}

