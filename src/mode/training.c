#define _POSIX_C_SOURCE 200809L /* mkdir, sigaction and clock_gettime are hidden under strict -std=c2x */

#include "training.h"

#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../training/candidate.h"
#include "../training/dashboard.h"
#include "../training/evaluator.h"
#include "../training/genetic.h"
#include "../training/results_csv.h"

/** Everything the callbacks of the genetic run need to know. */
typedef struct run {
    evaluator_t* evaluator;
    results_csv_t* csv;
    dashboard_t* dashboard;

    /* The best champion so far: the first one with the highest fitness. */
    bool has_best;
    double best_fitness;
    unsigned int best_generation;
    double* best_genome;
    unsigned int genome_length;

    unsigned int generations_done;
} run_t;

/**
 * Ctrl-C asks for a clean stop: the generation in progress finishes, its row is written, and the run
 * ends with the best genome so far. A second Ctrl-C quits at once.
 */
static volatile sig_atomic_t interrupts = 0;

static void on_interrupt(int signal_number);
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
static void show_progress(const evaluator_progress_t* progress, void* context);
static int report(const genetic_generation_t* generation, void* context);
static void print_summary(
    const run_t* run,
    config_human_player_t world,
    double elapsed,
    bool interrupted,
    const char* csv_path
);
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
    run_t run = { .csv = csv, .genome_length = length };
    run.best_genome = malloc(length * sizeof(double));
    if (min == NULL || max == NULL || run.best_genome == NULL) {
        printf("Error starting training: out of memory.\n");
        free(min);
        free(max);
        free(run.best_genome);
        return 1;
    }
    candidate_genome_ranges(config, min, max);

    int exit_code = 1;
    if (evaluator_create(config, training->population_size, &run.evaluator) != EVALUATOR_STATUS_OK) {
        printf("Error starting training: out of memory.\n");
        goto cleanup;
    }

    dashboard_info_t info = {
        .population_size = training->population_size,
        .generations = training->generations,
        .episodes_per_candidate = training->episodes_per_candidate,
        .episode_ticks = training->episode_ticks,
        .workers = training->workers,
        .seed = training->seed,
        .length_h = world.road_length_h,
        .length_v = world.road_length_v,
        .coeficient_max = training->coeficient_max,
        .threshold_max = training->threshold_max,
        .csv_path = csv_path,
    };
    unsigned int width = 0;
    dashboard_style_t style = dashboard_detect_style(stdout, training->workers, &width);
    if (!dashboard_create(stdout, style, width, &info, &run.dashboard)) {
        printf("Error starting training: out of memory.\n");
        goto cleanup;
    }
    evaluator_set_progress(run.evaluator, show_progress, &run);

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
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    struct sigaction handler = { .sa_handler = on_interrupt };
    struct sigaction previous;
    sigemptyset(&handler.sa_mask);
    interrupts = 0;
    sigaction(SIGINT, &handler, &previous);

    genetic_status_t status = genetic_run(&params, min, max, evaluate, report, &run, &result, NULL);

    sigaction(SIGINT, &previous, NULL);
    double elapsed = seconds_since(&start);
    bool interrupted = interrupts > 0 && status == GENETIC_STATUS_EREPORT;
    dashboard_finish(run.dashboard, status == GENETIC_STATUS_OK);

    if (status == GENETIC_STATUS_OK || interrupted) {
        print_summary(&run, world, elapsed, interrupted, csv_path);
        exit_code = 0;
    } else {
        printf("\nTraining stopped by an error: %s.\n", describe_failure(status));
    }

cleanup:
    dashboard_destroy(&run.dashboard);
    evaluator_destroy(&run.evaluator);
    free(min);
    free(max);
    free(run.best_genome);
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

/** The panel follows the workers while a generation is played. */
static void show_progress(const evaluator_progress_t* progress, void* context) {
    run_t* run = context;

    dashboard_update_progress(run->dashboard, progress);
}

/** One CSV row and one dashboard update per generation; a Ctrl-C makes this the last one. */
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

    dashboard_generation_t shown = {
        .index = generation->index,
        .champion_fitness = generation->best_fitness,
        .mean_fitness = generation->mean_fitness,
        .worst_fitness = generation->worst_fitness,
        .champion_cars_passed = champion->cars_passed,
        .champion_crashes = champion->crashes,
        .champion_genome = generation->best_genome,
    };
    dashboard_add_generation(run->dashboard, &shown);

    if (!run->has_best || generation->best_fitness > run->best_fitness) {
        run->has_best = true;
        run->best_fitness = generation->best_fitness;
        run->best_generation = generation->index;
        memcpy(run->best_genome, generation->best_genome, run->genome_length * sizeof(double));
    }
    run->generations_done++;

    return interrupts > 0 ? 1 : 0;
}

static void on_interrupt(int signal_number) {
    (void)signal_number;

    if (interrupts++ > 0) {
        dashboard_emergency_restore();
        _exit(130);
    }
}

static void print_summary(
    const run_t* run,
    config_human_player_t world,
    double elapsed,
    bool interrupted,
    const char* csv_path
) {
    if (interrupted) {
        printf("\nInterrupted after %u generation%s (%.1fs).\n", run->generations_done, run->generations_done == 1 ? "" : "s", elapsed);
    } else {
        printf("\nDone: %u generations in %.1fs.\n", run->generations_done, elapsed);
    }

    if (run->has_best) {
        printf(
            "Best genome seen (fitness %.2f, found in generation %u). For the [ai_player] section of config.ini:\n\n",
            run->best_fitness,
            run->best_generation + 1
        );
        printf("agent=linear_combination\nbinarization_method=threshold\nthreshold=%.10g\n", run->best_genome[0]);
        print_genes("coeficients_h", &run->best_genome[1], world.road_length_h);
        print_genes("coeficients_v", &run->best_genome[1 + world.road_length_h], world.road_length_v);
        printf("\n");
    }

    printf("The champion of every generation is in %s\n", csv_path);
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
