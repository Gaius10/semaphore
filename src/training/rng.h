#ifndef RNG_H
#define RNG_H

#include <stdint.h>

/**
 * A small random number generator (splitmix64) with its own state, for the training code. It never
 * touches rand(): the game draws its cars from rand() and reseeds it for every episode, so the
 * training must not share that state. The same seed always gives the same sequence, on any machine.
 *
 * The state is a single number, so an rng_t is cheap to keep on the stack. Not cryptographically
 * secure, which does not matter here.
 */
typedef struct rng {
    uint64_t state;
} rng_t;

/** Starts the generator from `seed`. Every seed is fine, including 0. */
void rng_init(rng_t* rng, uint64_t seed);

/** The next 64 random bits. */
uint64_t rng_next(rng_t* rng);

/** A number in [0, 1), made from the 53 high bits (what a double can hold). */
double rng_uniform(rng_t* rng);

/** A number in [0, n). n must be positive. The tiny bias of the modulo is far below what matters here. */
unsigned int rng_below(rng_t* rng, unsigned int n);

/** A standard normal number (mean 0, standard deviation 1), by the Box-Muller method. */
double rng_gaussian(rng_t* rng);

#endif
