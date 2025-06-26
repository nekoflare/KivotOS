//
// Created by neko on 6/21/25.
//

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stddef.h>
#include <sys/types.h>

#include "syscall_system.h"

ssize_t syscall_write(int fd, const void* buf, size_t count);

#endif //SYSCALLS_H
