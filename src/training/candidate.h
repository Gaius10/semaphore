#ifndef CANDIDATE_H
#define CANDIDATE_H

#include "../config/config.h"

/**
 * A candidate is one genome of the genetic algorithm: the numbers of a linear combination agent,
 *
 *     [threshold, coeficients_h[0..length_h-1], coeficients_v[0..length_v-1]]
 *
 * with the lengths being the road lengths of the config. Playing a candidate means running real,
 * headless games with the real agent (the one of AI mode), and scoring how well it did.
 */

typedef struct candidate_result {
    /** The average over the episodes of: cars passed - crash_penalty * crashes. */
    double fitness;
    /** The averages over the episodes of the two numbers the fitness is made of. */
    double cars_passed;
    double crashes;
} candidate_result_t;

typedef enum candidate_status {
    CANDIDATE_STATUS_OK,
    CANDIDATE_STATUS_ENOMEM, // Memory allocation failed.
    CANDIDATE_STATUS_EINVAL, // A NULL argument.
    CANDIDATE_STATUS_EPLAY,  // The game or the agent could not be built.
} candidate_status_t;

/** How many numbers a genome has: 1 + road_length_h + road_length_v. */
unsigned int candidate_genome_length(const config_t* config);

/**
 * Fills the range of every gene, candidate_genome_length numbers each: the threshold goes in
 * [0, threshold_max] and every coefficient in [0, coeficient_max] (so a waiting car can never make
 * its road look less urgent).
 */
void candidate_genome_ranges(const config_t* config, double* min, double* max);

/** The fitness of a game: cars_passed - crash_penalty * crashes. */
double candidate_fitness(double cars_passed, double crashes, double crash_penalty);

/**
 * Plays `episodes_per_candidate` games of `episode_ticks` ticks each with the genome as the agent,
 * and writes the averages to `result`. `config` is a training config: it gives the world, the
 * episodes, the crash penalty and the seed.
 *
 * Every game is a pure function of (config, genome, generation): before episode e the game's random
 * numbers are reset with srand(seed + generation * episodes_per_candidate + e). So two candidates in
 * the same generation play exactly the same traffic, which makes their fitness comparable, and the
 * traffic changes between generations, so the search can't overfit one sequence of cars.
 *
 * Because of that srand call, this function changes the rand() state of the calling process. The
 * parallel evaluator calls it only in worker processes, each with its own state.
 *
 * `result` is untouched on failure.
 */
candidate_status_t candidate_play(
    const config_t* config,
    const double* genome,
    unsigned int generation,
    candidate_result_t* result
);

#endif
