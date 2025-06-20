//
// Created by neko on 6/7/25.
//

#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limine.h>
#include <fcntl.h>

void vfs_init();

int  open (const char *__file, int __oflag, ...);
void close (int __fd);
int  read (int __fd, void *__buf, size_t __nbytes);
int  fstat (int __fd, struct stat *__buf);

#endif //VFS_H
