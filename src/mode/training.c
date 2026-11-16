#define _POSIX_C_SOURCE 200809L /* mkdir and clock_gettime are hidden under strict -std=c2x */

#include "training.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "../training/candidate.h"
#include "../training/evaluator.h"
#include "../training/genetic.h"
#include "../training/results_csv.h"

/** Everything the callbacks of the genetic run need to know. */
typedef struct run {
    evaluator_t* evaluator;
    results_csv_t* csv;
    unsigned int generations;
    struct timespec start;
} run_t;

static int run_training(
    const config_t* config,
    const config_training_t* training,
    results_csv_t* csv,
    const char* csv_path
);
static int ensure_directory(const char* path);
static int evaluate(
    const double* genomes,
    unsigned int count,
    unsigned int genome_length,
    unsigned int generation,
    double* fitness_out,
    void* context
);
static int report(const genetic_generation_t* generation, void* context);
static const char* describe_failure(genetic_status_t status);
static double seconds_since(const struct timespec* start);
static void print_genes(const char* name, const double* genes, unsigned int count);

int mode_run_training(const config_t* config) {
    config_training_t training = config_get_training(config);
    config_human_player_t world = config_get_human_player(config);

    if (ensure_directory(training.output_dir) != 0) {
        printf("Error creating the folder '%s' for the results.\n", training.output_dir);
        return 1;
    }

    char csv_path[512];
    if (results_csv_default_path(training.output_dir, time(NULL), csv_path, sizeof(csv_path)) != 0) {
        printf("Error: the results path is too long.\n");
        return 1;
    }

    results_csv_t* csv = NULL;
    switch (results_csv_open(csv_path, world.road_length_h, world.road_length_v, &csv)) {
        case RESULTS_CSV_STATUS_OK:
            break;
        case RESULTS_CSV_STATUS_EEXIST:
            printf("Error: '%s' already exists, and a results file is never overwritten. Try again in a second.\n", csv_path);
            return 1;
        case RESULTS_CSV_STATUS_ENOMEM:
            printf("Error creating the results file: out of memory.\n");
            return 1;
        default:
            printf("Error creating the results file '%s'.\n", csv_path);
            return 1;
    }

    int exit_code = run_training(config, &training, csv, csv_path);

    if (results_csv_close(&csv) != RESULTS_CSV_STATUS_OK) {
        printf("Error writing the results file '%s'.\n", csv_path);
        exit_code = 1;
    }

    return exit_code;
}

static int run_training(
    const config_t* config,
    const config_training_t* training,
    results_csv_t* csv,
    const char* csv_path
) {
    unsigned int length = candidate_genome_length(config);
    config_human_player_t world = config_get_human_player(config);

    double* min = malloc(length * sizeof(double));
    double* max = malloc(length * sizeof(double));
    double* best = malloc(length * sizeof(double));
    if (min == NULL || max == NULL || best == NULL) {
        printf("Error starting training: out of memory.\n");
        free(min);
        free(max);
        free(best);
        return 1;
    }
    candidate_genome_ranges(config, min, max);

    run_t run = { .evaluator = NULL, .csv = csv, .generations = training->generations };
    if (evaluator_create(config, training->population_size, &run.evaluator) != EVALUATOR_STATUS_OK) {
        printf("Error starting training: out of memory.\n");
        free(min);
        free(max);
        free(best);
        return 1;
    }

    genetic_params_t params = {
        .population_size = training->population_size,
        .genome_length = length,
        .generations = training->generations,
        .elite_count = training->elite_count,
        .tournament_size = training->tournament_size,
        .mutation_rate = training->mutation_rate,
        .mutation_sigma = training->mutation_sigma,
        .seed = training->seed,
    };
    genetic_result_t result;

    printf(
        "Training: %u candidates x %u generations, %u episodes of %u ticks each, seed %u, %u worker%s.\n",
        training->population_size,
        training->generations,
        training->episodes_per_candidate,
        training->episode_ticks,
        training->seed,
        training->workers,
        training->workers == 1 ? "" : "s"
    );
    printf("Results: %s\n\n", csv_path);
    fflush(stdout);

    clock_gettime(CLOCK_MONOTONIC, &run.start);
    genetic_status_t status = genetic_run(&params, min, max, evaluate, report, &run, &result, best);
    double elapsed = seconds_since(&run.start);

    int exit_code = 0;
    if (status == GENETIC_STATUS_OK) {
        printf("\nDone: %u generations in %.1fs.\n", training->generations, elapsed);
        printf(
            "Best genome seen (fitness %.2f, found in generation %u). For the [ai_player] section of config.ini:\n\n",
            result.best_fitness,
            result.best_generation
        );
        printf("agent=linear_combination\nbinarization_method=threshold\nthreshold=%.10g\n", best[0]);
        print_genes("coeficients_h", &best[1], world.road_length_h);
        print_genes("coeficients_v", &best[1 + world.road_length_h], world.road_length_v);
        printf("\nThe champion of every generation is in %s\n", csv_path);
    } else {
        printf("\nTraining stopped by an error: %s.\n", describe_failure(status));
        exit_code = 1;
    }

    evaluator_destroy(&run.evaluator);
    free(min);
    free(max);
    free(best);
    return exit_code;
}

/** Creates `path` if it is not there yet (one level: its parent must exist). */
static int ensure_directory(const char* path) {
    if (mkdir(path, 0777) == 0 || errno == EEXIST) {
        return 0;
    }

    return 1;
}

/** genetic_run gives one context to both callbacks, so the evaluator travels inside it. */
static int evaluate(
    const double* genomes,
    unsigned int count,
    unsigned int genome_length,
    unsigned int generation,
    double* fitness_out,
    void* context
) {
    run_t* run = context;

    return evaluator_evaluate(genomes, count, genome_length, generation, fitness_out, run->evaluator);
}

/** One CSV row and one line of text per generation. */
static int report(const genetic_generation_t* generation, void* context) {
    run_t* run = context;
    const candidate_result_t* champion = evaluator_result(run->evaluator, generation->best_index);
    if (champion == NULL) {
        return 1;
    }

    results_row_t row = {
        .generation = generation->index,
        .champion_fitness = generation->best_fitness,
        .mean_fitness = generation->mean_fitness,
        .worst_fitness = generation->worst_fitness,
        .champion_cars_passed = champion->cars_passed,
        .champion_crashes = champion->crashes,
        .genome = generation->best_genome,
    };
    if (results_csv_write(run->csv, &row) != RESULTS_CSV_STATUS_OK) {
        return 1;
    }

    printf(
        "generation %*u/%u  champion %8.2f  mean %8.2f  worst %8.2f  (cars %.1f, crashes %.2f)  %6.1fs\n",
        (int)snprintf(NULL, 0, "%u", run->generations),
        generation->index + 1,
        run->generations,
        generation->best_fitness,
        generation->mean_fitness,
        generation->worst_fitness,
        champion->cars_passed,
        champion->crashes,
        seconds_since(&run->start)
    );
    fflush(stdout);
    return 0;
}

static const char* describe_failure(genetic_status_t status) {
    switch (status) {
        case GENETIC_STATUS_EEVALUATE:
            return "a candidate could not be played";
        case GENETIC_STATUS_EREPORT:
            return "the results file could not be written";
        case GENETIC_STATUS_ENOMEM:
            return "out of memory";
        default:
            return "the training settings were rejected";
    }
}

static double seconds_since(const struct timespec* start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)(now.tv_sec - start->tv_sec) + (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}

static void print_genes(const char* name, const double* genes, unsigned int count) {
    printf("%s=", name);
    for (unsigned int i = 0; i < count; i++) {
        printf("%s%.10g", i == 0 ? "" : ", ", genes[i]);
    }
    printf("\n");
}
