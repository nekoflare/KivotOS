//
// Created by neko on 6/8/25.
//

#include <errno.h>

int kErrno = 0;

int * __errno_location(void) {
    return &kErrno; // todo: make it thread local.
}