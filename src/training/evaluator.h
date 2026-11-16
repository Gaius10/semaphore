#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "../config/config.h"
#include "candidate.h"

/**
 * Scores whole populations for the genetic algorithm (see genetic_evaluate_fn in genetic.h): it plays
 * every candidate with candidate_play and hands back the fitness. The candidates are played in
 * parallel, by a pool of worker processes.
 *
 * Why processes and not threads: the game draws its cars from rand(), and the hidden state of rand()
 * is shared by all the threads of a process. A worker process has a private copy of it, so it can
 * srand() before each episode and get exactly the traffic a serial run would get, however many
 * workers there are and however the system schedules them. The results come back through a block of
 * shared memory.
 *
 * Every call to evaluator_evaluate forks the workers (one fork per generation is nothing next to the
 * games), worker w plays the candidates w, w + workers, w + 2 * workers... and exits. The result of a
 * candidate depends only on (config, genome, generation), so it is the same for any number of workers.
 * Even a single worker is a separate process, so there is only one code path.
 *
 * The evaluator also keeps the full result of every candidate of the last population it scored, so
 * that the caller can read the cars passed and the crashes of, say, the champion.
 */
typedef struct evaluator evaluator_t;

typedef enum evaluator_status {
    EVALUATOR_STATUS_OK,
    EVALUATOR_STATUS_ENOMEM, // Memory allocation failed.
    EVALUATOR_STATUS_EINVAL, // A NULL argument, or a capacity or a number of workers of 0.
} evaluator_status_t;

/**
 * How one candidate is played. It is called in a worker process, once per candidate, and it must give
 * the same result for the same arguments. Anything but CANDIDATE_STATUS_OK fails the whole evaluation.
 */
typedef candidate_status_t (*evaluator_play_fn)(
    const config_t* config,
    const double* genome,
    unsigned int generation,
    candidate_result_t* result
);

/** How far the workers are, while a population is being scored. The arrays are only valid during the call. */
typedef struct evaluator_progress {
    unsigned int generation;
    /** Candidates in the population, and how many are done. */
    unsigned int total;
    unsigned int done;
    unsigned int workers;
    /** Per worker: how many candidates it was given, and how many it has finished. */
    const unsigned int* assigned;
    const unsigned int* finished;
} evaluator_progress_t;

/**
 * Called by the process that called evaluator_evaluate, while it waits for the workers: once when they
 * start (nothing done), about every 50 ms while they work, and once when they are all done.
 */
typedef void (*evaluator_progress_fn)(const evaluator_progress_t* progress, void* context);

/**
 * Builds an evaluator for populations of up to `capacity` candidates, for the training `config`, with
 * the `workers` of the config. The config must outlive the evaluator. On success, *evaluator must be
 * released with evaluator_destroy.
 */
evaluator_status_t evaluator_create(const config_t* config, unsigned int capacity, evaluator_t** evaluator);

/** The same, with the number of workers and the way to play a candidate given (candidate_play is the usual one). */
evaluator_status_t evaluator_create_with(
    const config_t* config,
    unsigned int capacity,
    unsigned int workers,
    evaluator_play_fn play,
    evaluator_t** evaluator
);

void evaluator_destroy(evaluator_t** evaluator);

/** Sets (or, with NULL, clears) the progress callback. */
void evaluator_set_progress(evaluator_t* evaluator, evaluator_progress_fn progress, void* context);

/**
 * A genetic_evaluate_fn whose `context` must be the evaluator. (genetic_run gives one context to both
 * callbacks, so a caller that needs its own context for the report calls this from a small
 * evaluate function of its own, passing the evaluator.) Writes the fitness of each genome to
 * fitness_out and keeps the results for evaluator_result.
 *
 * Returns non-zero if the shape does not fit (a genome length that is not candidate_genome_length, or
 * more candidates than the capacity), if a worker can't be started, or if a worker fails (it could
 * not play its candidate, or it died). Then the other workers are killed, every worker is reaped (none
 * is left running or as a zombie), and there are no results. The evaluator can be used again.
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
