#include "rng.h"

#include <math.h>

void rng_init(rng_t* rng, uint64_t seed) {
    rng->state = seed;
}

/**
 * The state moves by a fixed odd constant (the golden ratio scaled to 64 bits) on every call, and the
 * output is that counter scrambled: xor-shifts bring high bits down, and the multiplications by large
 * odd constants spread every bit change across the whole word. The constants are the published
 * splitmix64 ones (Sebastiano Vigna), picked so the output passes statistical tests: do not change them.
 */
uint64_t rng_next(rng_t* rng) {
    rng->state += 0x9E3779B97F4A7C15ULL;
    uint64_t z = rng->state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

double rng_uniform(rng_t* rng) {
    return (double)(rng_next(rng) >> 11) * (1.0 / 9007199254740992.0);
}

unsigned int rng_below(rng_t* rng, unsigned int n) {
    return (unsigned int)(rng_next(rng) % n);
}

/** The second number Box-Muller makes is thrown away, to keep the state to one number. */
double rng_gaussian(rng_t* rng) {
    const double two_pi = 6.283185307179586;
    double u1 = 1.0 - rng_uniform(rng); // (0, 1], so log() is finite
    double u2 = rng_uniform(rng);
    return sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
}
