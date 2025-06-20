//
// Created by neko on 6/15/25.
//

#include "mutex.h"

void lock_mutex(struct mutex *mtx) {
    while (__sync_lock_test_and_set(&mtx->locked, 1)) {
        asm volatile ("nop");
    }
}

void unlock_mutex(struct mutex *mtx) {
    __sync_lock_release(&mtx->locked);
}
