#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>      // for error codes like -EFAULT, -EBADF, -ENOMEM
#include <mem/gheap.h>
#include <mem/virtual.h>
#include <x86/log.h>

#include "../syscalls.h"

#define PAGE_SIZE 4096

#define ALIGN_DOWN(x, align) ((uintptr_t)(x) & ~((align) - 1))

// Check if user memory [addr, addr+size) is valid and readable/writable
static int check_user_memory(const void *addr, size_t size) {
    if (size == 0) return 0;

    uintptr_t start = (uintptr_t) addr;
    uintptr_t end = start + size - 1;
    if (end < start) // overflow check
        return -1;

    uintptr_t current = ALIGN_DOWN(start, PAGE_SIZE);
    while (current <= end) {
        if (!is_rw((void *)current)) {
            return -1;
        }
        current += PAGE_SIZE;
    }
    return 0;
}

// Copy from user buffer to kernel buffer safely, returns 0 on success, negative on failure
static int memcpy_u2k(void *dest, const void *src, size_t count) {
    if (check_user_memory(src, count) < 0) {
        return -EFAULT;  // Bad address
    }

    char *d = (char *)dest;
    const char *s = (const char *)src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return 0;
}

// syscall_write returns number of bytes written or negative error code
ssize_t syscall_write(int fd, const void* buf, size_t count) {
    fd=1; // bruh fk this fd vro
    if (count == 0) {
        return 0;
    }

    // only fd 1 or 2 cuz its stdout or stderr.
    if (fd != 1 && fd != 2) {
        return -EBADF; // Bad file descriptor
    }

    char *buffer = malloc(count + 1);
    if (!buffer) {
        return -ENOMEM; // Out of memory !?!?
    }

    int err = memcpy_u2k(buffer, buf, count);
    if (err < 0) {
        free(buffer);
        return err; // goodnight baby..
    }

    buffer[count] = '\0'; // null terminate

    disable_printing_time(); // todo: first check if we're printing time in the first place :3
    debug_log(buffer);
    enable_printing_time();

    free(buffer);
    return (ssize_t)count;
}
