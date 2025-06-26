//
// Created by neko on 6/23/25.
//

#ifndef INTERRUPTS_HELPERS_H
#define INTERRUPTS_HELPERS_H

static inline unsigned long get_cpu_flags(void) {
    unsigned long flags;
    asm volatile (
        "pushfq\n\t"
        "popq %0\n\t"
        : "=r" (flags)
        :
        : "memory"
    );
    return flags;
}

static inline unsigned long save_and_clear_if(void) {
    unsigned long flags;
    asm volatile (
        "pushfq\n\t"
        "popq %0\n\t"
        "cli\n\t"              // clear IF
        : "=r" (flags)
        :
        : "memory"
    );
    return flags;
}

static inline void restore_flags(unsigned long flags) {
    asm volatile (
        "pushq %0\n\t"
        "popfq\n\t"
        :
        : "r" (flags)
        : "memory", "cc"
    );
}

#endif //INTERRUPTS_HELPERS_H
