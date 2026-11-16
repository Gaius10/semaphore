#include "genetic.h"

// TODO(refactor): see the note in genetic.h. The overall design of this library is to be reworked later.

#include "rng.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool params_are_valid(const genetic_params_t* p, const double* min, const double* max);
static void random_population(
    double* genomes,
    const genetic_params_t* p,
    const double* min,
    const double* max,
    rng_t* rng
);
static unsigned int tournament(const double* fitness, unsigned int size, unsigned int rounds, rng_t* rng);
static const double* pick_parent(
    const double* genomes,
    const double* fitness,
    const genetic_params_t* p,
    rng_t* rng
);
static void breed(
    const genetic_params_t* p,
    const double* min,
    const double* max,
    const double* genomes,
    const double* fitness,
    double* next,
    bool* taken,
    rng_t* rng
);
static unsigned int rank_best(const double* fitness, unsigned int count, const bool* taken);
static bool is_better(double a, double b);

genetic_status_t genetic_run(
    const genetic_params_t* params,
    const double* min,
    const double* max,
    genetic_evaluate_fn evaluate,
    genetic_report_fn report,
    void* context,
    genetic_result_t* result,
    double* best_genome
) {
    if (
        params == NULL
        || min == NULL
        || max == NULL
        || evaluate == NULL
        || result == NULL
        || !params_are_valid(params, min, max)
    ) {
        return GENETIC_STATUS_EINVAL;
    }

    unsigned int count = params->population_size;
    unsigned int length = params->genome_length;
    size_t genome_numbers = (size_t)count * length;

    double* genomes = malloc(genome_numbers * sizeof(double));
    double* next = malloc(genome_numbers * sizeof(double));
    double* fitness = malloc(count * sizeof(double));
    double* seen_genome = malloc(length * sizeof(double)); // best genome of the whole run
    bool* taken = malloc(count * sizeof(bool)); // scratch for picking the elites
    if (genomes == NULL || next == NULL || fitness == NULL || seen_genome == NULL || taken == NULL) {
        free(genomes);
        free(next);
        free(fitness);
        free(seen_genome);
        free(taken);
        return GENETIC_STATUS_ENOMEM;
    }

    rng_t rng;
    rng_init(&rng, params->seed);
    random_population(genomes, params, min, max, &rng);

    genetic_status_t status = GENETIC_STATUS_OK;
    double seen_fitness = NAN;
    unsigned int seen_generation = 0;

    for (unsigned int generation = 0; generation < params->generations; generation++) {
        if (evaluate(genomes, count, length, generation, fitness, context) != 0) {
            status = GENETIC_STATUS_EEVALUATE;
            break;
        }

        bool has_nan = false;
        double sum = 0.0;
        double worst = fitness[0];
        for (unsigned int i = 0; i < count; i++) {
            has_nan = has_nan || isnan(fitness[i]);
            sum += fitness[i];
            if (fitness[i] < worst) {
                worst = fitness[i];
            }
        }

        if (has_nan) {
            status = GENETIC_STATUS_EEVALUATE;
            break;
        }

        unsigned int best_index = rank_best(fitness, count, NULL);

        if (isnan(seen_fitness) || fitness[best_index] > seen_fitness) {
            seen_fitness = fitness[best_index];
            seen_generation = generation;
            memcpy(seen_genome, &genomes[(size_t)best_index * length], length * sizeof(double));
        }

        if (report != NULL) {
            genetic_generation_t info = {
                .index = generation,
                .best_fitness = fitness[best_index],
                .best_index = best_index,
                .mean_fitness = sum / count,
                .worst_fitness = worst,
                .genomes = genomes,
                .fitness = fitness,
                .best_genome = &genomes[(size_t)best_index * length],
            };

            if (report(&info, context) != 0) {
                status = GENETIC_STATUS_EREPORT;
                break;
            }
        }

        if (generation + 1 < params->generations) {
            breed(params, min, max, genomes, fitness, next, taken, &rng);

            double* swap = genomes;
            genomes = next;
            next = swap;
        }
    }

    if (status == GENETIC_STATUS_OK) {
        result->best_fitness = seen_fitness;
        result->best_generation = seen_generation;
        if (best_genome != NULL) {
            memcpy(best_genome, seen_genome, length * sizeof(double));
        }
    }

    free(genomes);
    free(next);
    free(fitness);
    free(seen_genome);
    free(taken);
    return status;
}

static bool params_are_valid(const genetic_params_t* p, const double* min, const double* max) {
    if (p->population_size < 2 || p->genome_length < 1 || p->generations < 1) {
        return false;
    }

    if (p->elite_count >= p->population_size) {
        return false;
    }

    if (p->tournament_size < 1 || p->tournament_size > p->population_size) {
        return false;
    }

    if (!(p->mutation_rate >= 0.0 && p->mutation_rate <= 1.0)) { // also false for NaN
        return false;
    }

    if (!(p->mutation_sigma > 0.0) || !isfinite(p->mutation_sigma)) {
        return false;
    }

    for (unsigned int i = 0; i < p->genome_length; i++) {
        if (!isfinite(min[i]) || !isfinite(max[i]) || min[i] > max[i]) {
            return false;
        }
    }

    return true;
}

static void random_population(
    double* genomes,
    const genetic_params_t* p,
    const double* min,
    const double* max,
    rng_t* rng
) {
    for (unsigned int i = 0; i < p->population_size; i++) {
        for (unsigned int g = 0; g < p->genome_length; g++) {
            genomes[(size_t)i * p->genome_length + g] = min[g] + rng_uniform(rng) * (max[g] - min[g]);
        }
    }
}

/** True when `a` is a strictly better fitness than `b`. */
static bool is_better(double a, double b) {
    return a > b;
}

/**
 * The index of the best fitness among the candidates that are not `taken` (which may be NULL). Ties
 * go to the lowest index, so the choice does not depend on anything but the fitness.
 */
static unsigned int rank_best(const double* fitness, unsigned int count, const bool* taken) {
    unsigned int best = count;
    for (unsigned int i = 0; i < count; i++) {
        if (taken != NULL && taken[i]) {
            continue;
        }

        if (best == count || is_better(fitness[i], fitness[best])) {
            best = i;
        }
    }

    return best;
}

/** Draws `rounds` candidates at random (with replacement) and returns the best of them. */
static unsigned int tournament(const double* fitness, unsigned int size, unsigned int rounds, rng_t* rng) {
    unsigned int winner = rng_below(rng, size);
    for (unsigned int i = 1; i < rounds; i++) {
        unsigned int challenger = rng_below(rng, size);
        if (is_better(fitness[challenger], fitness[winner])) {
            winner = challenger;
        }
    }

    return winner;
}

/** The genome of a parent chosen by tournament selection. */
static const double* pick_parent(
    const double* genomes,
    const double* fitness,
    const genetic_params_t* p,
    rng_t* rng
) {
    unsigned int winner = tournament(fitness, p->population_size, p->tournament_size, rng);
    return &genomes[(size_t)winner * p->genome_length];
}

/**
 * Fills `next` from `genomes`: the elites first, then children. `taken` is scratch space (one flag
 * per candidate).
 */
static void breed(
    const genetic_params_t* p,
    const double* min,
    const double* max,
    const double* genomes,
    const double* fitness,
    double* next,
    bool* taken,
    rng_t* rng
) {
    unsigned int count = p->population_size;
    unsigned int length = p->genome_length;

    // The elites go first, best first.
    memset(taken, 0, count * sizeof(bool));
    for (unsigned int e = 0; e < p->elite_count; e++) {
        unsigned int best = rank_best(fitness, count, taken);
        taken[best] = true;
        memcpy(&next[(size_t)e * length], &genomes[(size_t)best * length], length * sizeof(double));
    }

    for (unsigned int c = p->elite_count; c < count; c++) {
        const double* parent_a = pick_parent(genomes, fitness, p, rng);
        const double* parent_b = pick_parent(genomes, fitness, p, rng);
        double* child = &next[(size_t)c * length];

        for (unsigned int g = 0; g < length; g++) {
            double gene = (rng_next(rng) & 1) ? parent_a[g] : parent_b[g];

            if (rng_uniform(rng) < p->mutation_rate) {
                gene += rng_gaussian(rng) * p->mutation_sigma * (max[g] - min[g]);
            }

            child[g] = fmin(fmax(gene, min[g]), max[g]);
        }
    }
}
