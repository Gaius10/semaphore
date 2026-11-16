#ifndef GENETIC_H
#define GENETIC_H

#include <stdint.h>

/*
 * TODO(refactor): the overall design of this library (API shape, how it is split between the engine,
 * the callbacks and the helpers) is not final and needs to be reworked. It is kept as is for now so
 * that training mode can be built on top of it: do not polish it further until the refactor.
 */

/**
 * A generic genetic algorithm over real-valued genomes. It knows nothing about the game: the caller
 * says how good a population is through genetic_evaluate_fn, and the engine breeds the next one.
 *
 * Every candidate (genome) is genome_length numbers, each kept inside its own [min, max] range. The
 * first population is random. After each generation the next one is built like this:
 *
 *  1. The elite_count best candidates are copied unchanged into the first slots, best first.
 *  2. Each other slot gets a child: two parents are picked by tournament selection (the best of
 *     tournament_size candidates drawn at random), every gene is taken from either parent with equal
 *     chance (uniform crossover), and then each gene mutates with probability mutation_rate, by a
 *     Gaussian step with a standard deviation of mutation_sigma times the range of that gene.
 *     Genes are clamped back into their ranges.
 *
 * The engine has its own random number generator, seeded from the params, and never touches rand():
 * the same params and the same (deterministic) fitness always give the same run.
 */

typedef struct genetic_params {
    /** Candidates per generation. At least 2. */
    unsigned int population_size;
    /** Numbers in one genome. At least 1. */
    unsigned int genome_length;
    /** How many generations to run. At least 1. */
    unsigned int generations;
    /** Best candidates copied unchanged into the next generation. Less than population_size. */
    unsigned int elite_count;
    /** Candidates drawn for each parent selection. From 1 to population_size. */
    unsigned int tournament_size;
    /** Chance that each gene of a child mutates, from 0 to 1. */
    double mutation_rate;
    /** Size of a mutation, as a fraction of the range of the gene. Finite and above 0. */
    double mutation_sigma;
    uint64_t seed;
} genetic_params_t;

typedef enum genetic_status {
    GENETIC_STATUS_OK,
    GENETIC_STATUS_ENOMEM,    // Memory allocation failed.
    GENETIC_STATUS_EINVAL,    // The params or the ranges are not valid.
    GENETIC_STATUS_EEVALUATE, // The evaluate callback failed or gave a NaN fitness.
    GENETIC_STATUS_EREPORT,   // The report callback asked to stop.
} genetic_status_t;

/**
 * Scores a whole population at once, so that the caller is free to do it in parallel.
 *
 * `genomes` holds `count` genomes of `genome_length` numbers each, one after the other, and the
 * fitness of genome i goes in fitness_out[i]. Higher is better, and it must not be NaN. `generation`
 * starts at 0. Returns 0 on success; any other value stops the run with GENETIC_STATUS_EEVALUATE.
 */
typedef int (*genetic_evaluate_fn)(
    const double* genomes,
    unsigned int count,
    unsigned int genome_length,
    unsigned int generation,
    double* fitness_out,
    void* context
);

/**
 * Everything about a generation that has just been evaluated. The pointers are only valid during
 * the report callback.
 */
typedef struct genetic_generation {
    unsigned int index;
    /** Fitness of the champion (the best candidate of this generation), and where it is in `genomes`. */
    double best_fitness;
    unsigned int best_index;
    double mean_fitness;
    double worst_fitness;
    /** The whole population that was evaluated, and its fitness: population_size genomes. */
    const double* genomes;
    const double* fitness;
    /** The champion's genome: genome_length numbers, the same as genomes[best_index]. */
    const double* best_genome;
} genetic_generation_t;

/**
 * Called after every generation is evaluated. Returns 0 to go on; any other value stops the run with
 * GENETIC_STATUS_EREPORT.
 */
typedef int (*genetic_report_fn)(const genetic_generation_t* generation, void* context);

typedef struct genetic_result {
    /** The best fitness seen in the whole run, and the generation it was seen in (the first one on a tie). */
    double best_fitness;
    unsigned int best_generation;
} genetic_result_t;

/**
 * Runs the algorithm. `min` and `max` give the range of each gene (genome_length numbers each, finite,
 * min <= max). `report` may be NULL.
 *
 * On success, `result` is filled and, if `best_genome` is not NULL, it receives the best genome seen
 * in the whole run (genome_length numbers). If the fitness depends on the generation (e.g. the
 * traffic changes each generation), fitness values of different generations are not strictly
 * comparable, so this is "best seen", not "best possible".
 *
 * On any failure nothing is leaked, and `result` and `best_genome` are left as they were. If the run
 * is stopped by a callback, the generations that were already reported stay reported.
 */
genetic_status_t genetic_run(
    const genetic_params_t* params,
    const double* min,
    const double* max,
    genetic_evaluate_fn evaluate,
    genetic_report_fn report,
    void* context,
    genetic_result_t* result,
    double* best_genome
);

#endif
