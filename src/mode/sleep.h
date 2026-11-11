#ifndef MODE_SLEEP_H
#define MODE_SLEEP_H

#include <threads.h>
#include <time.h>

/** Sleeps the calling thread for `ms` milliseconds. Shared by the modes to pace their ticks. */
static inline void sleep_ms(unsigned int ms) {
    struct timespec time = { .tv_sec = ms / 1000, .tv_nsec = (long) (ms % 1000) * 1000000L };
    thrd_sleep(&time, NULL);
}

#endif
