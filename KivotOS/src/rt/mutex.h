//
// Created by neko on 6/15/25.
//

#ifndef MUTEX_H
#define MUTEX_H

struct mutex {
    volatile int locked;
};

void lock_mutex(struct mutex* mtx);
void unlock_mutex(struct mutex* mtx);

#endif //MUTEX_H
