//
// Created by neko on 6/15/25.
//

#include <x86/interrupts_helpers.h>
#include "mutex.h"

void lock_mutex(struct mutex *mtx) {
    unsigned long flags = save_and_clear_if();
    while (__sync_lock_test_and_set(&mtx->locked, 1)) {
        asm volatile ("nop");
    }
    restore_flags(flags);
}

void unlock_mutex(struct mutex *mtx) {
    __sync_lock_release(&mtx->locked);
}
