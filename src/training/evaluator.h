#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "../config/config.h"
#include "candidate.h"

/**
 * Scores whole populations for the genetic algorithm (see genetic_evaluate_fn in genetic.h): it plays
 * every candidate with candidate_play and hands back the fitness. This is the serial version, one
 * candidate after another in the calling process.
 *
 * The evaluator also keeps the full result of every candidate of the last population it scored, so
 * that the caller can read the cars passed and the crashes of, say, the champion.
 */
typedef struct evaluator evaluator_t;

typedef enum evaluator_status {
    EVALUATOR_STATUS_OK,
    EVALUATOR_STATUS_ENOMEM, // Memory allocation failed.
    EVALUATOR_STATUS_EINVAL, // A NULL argument or a capacity of 0.
} evaluator_status_t;

/**
 * Builds an evaluator for populations of up to `capacity` candidates, for the training `config`. The
 * config must outlive the evaluator. On success, *evaluator must be released with evaluator_destroy.
 */
evaluator_status_t evaluator_create(const config_t* config, unsigned int capacity, evaluator_t** evaluator);
void evaluator_destroy(evaluator_t** evaluator);

/**
 * A genetic_evaluate_fn whose `context` must be the evaluator. (genetic_run gives one context to both
 * callbacks, so a caller that needs its own context for the report calls this from a small
 * evaluate function of its own, passing the evaluator.) Writes the fitness of each genome to
 * fitness_out and keeps the results for evaluator_result. Returns non-zero (and leaves the results
 * unspecified) if the shape does not fit (a genome length that is not candidate_genome_length, or
 * more candidates than the capacity) or if a candidate can't be played.
 */
int evaluator_evaluate(
    const double* genomes,
    unsigned int count,
    unsigned int genome_length,
    unsigned int generation,
    double* fitness_out,
    void* context
);

/** The full result of candidate `index` of the last population scored, or NULL if there is none. */
const candidate_result_t* evaluator_result(const evaluator_t* evaluator, unsigned int index);

#endif
