#define _POSIX_C_SOURCE 200809L /* mkstemp is hidden under strict -std=c2x */

#include <check.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/agents/ai.h"
#include "../src/config/config.h"
#include "../src/game/game.h"
#include "../src/training/candidate.h"
#include "../src/training/evaluator.h"
#include "../src/training/genetic.h"
#include "../src/training/rng.h"
#include "suites.h"

/**
 * Tests of the candidate wrapper and the serial evaluator, with the real game: a training config
 * with a 10 x 12 world, busy roads (0.5 spawn probability) and a few short episodes.
 */

#define WORLD_H 10
#define WORLD_V 12
#define GENOME_LENGTH (1 + WORLD_H + WORLD_V)
#define THRESHOLD_MAX 4.0
#define COEFICIENT_MAX 2.0
#define POPULATION 6

static char config_path[64];
static config_t* config;

/** Writes a training config to a temp file and builds `config` from it. */
static void make_config(unsigned int episodes, unsigned int ticks, double crash_penalty, unsigned int seed) {
    snprintf(config_path, sizeof(config_path), "/tmp/semaphore_evaluator_XXXXXX");
    int fd = mkstemp(config_path);
    ck_assert_int_ge(fd, 0);
    close(fd);

    FILE* f = fopen(config_path, "w");
    ck_assert_ptr_nonnull(f);
    fprintf(
        f,
        "mode=training\n"
        "car_spawn_probability_h=0.5\n"
        "car_spawn_probability_v=0.5\n"
        "road_length_h=%d\n"
        "road_length_v=%d\n"
        "population_size=%d\n"
        "generations=4\n"
        "episodes_per_candidate=%u\n"
        "episode_ticks=%u\n"
        "crash_penalty=%g\n"
        "elite_count=1\n"
        "tournament_size=2\n"
        "mutation_rate=0.2\n"
        "mutation_sigma=0.2\n"
        "coeficient_max=%g\n"
        "threshold_max=%g\n"
        "seed=%u\n"
        "workers=1\n"
        "training_output_dir=data\n",
        WORLD_H, WORLD_V, POPULATION, episodes, ticks, crash_penalty, COEFICIENT_MAX, THRESHOLD_MAX, seed
    );
    fclose(f);

    config = NULL;
    ck_assert_int_eq(config_create(config_path, &config), CONFIG_STATUS_OK);
}

static void teardown(void) {
    config_destroy(&config);
    unlink(config_path);
}

static void setup(void) {
    make_config(3, 300, 5.0, 100);
}

/** The agent of the hand-written config: it switches when a road is clearly busier. */
static void reasonable_genome(double* genome) {
    genome[0] = 0.1;
    for (int g = 1; g < GENOME_LENGTH; g++) {
        genome[g] = 1.0;
    }
}

/** Zero coefficients and the biggest margin: the lights never change after the first tick. */
static void never_switching_genome(double* genome) {
    genome[0] = THRESHOLD_MAX;
    for (int g = 1; g < GENOME_LENGTH; g++) {
        genome[g] = 0.0;
    }
}

static void random_genome(rng_t* rng, double* genome) {
    genome[0] = rng_uniform(rng) * THRESHOLD_MAX;
    for (int g = 1; g < GENOME_LENGTH; g++) {
        genome[g] = rng_uniform(rng) * COEFICIENT_MAX;
    }
}

static candidate_result_t play(const double* genome, unsigned int generation) {
    candidate_result_t result;
    ck_assert_int_eq(candidate_play(config, genome, generation, &result), CANDIDATE_STATUS_OK);
    return result;
}

/* ---------------------------------------------------------------- candidate */

START_TEST(test_candidate_genome_has_a_threshold_and_one_gene_per_road_cell) {
    ck_assert_uint_eq(candidate_genome_length(config), 1 + WORLD_H + WORLD_V);
}
END_TEST

START_TEST(test_candidate_ranges_follow_the_config) {
    double min[GENOME_LENGTH], max[GENOME_LENGTH];
    candidate_genome_ranges(config, min, max);

    ck_assert_double_eq(min[0], 0.0);
    ck_assert_double_eq(max[0], THRESHOLD_MAX);
    for (int g = 1; g < GENOME_LENGTH; g++) {
        ck_assert_double_eq(min[g], 0.0);
        ck_assert_double_eq(max[g], COEFICIENT_MAX);
    }
}
END_TEST

START_TEST(test_candidate_fitness_is_cars_minus_penalized_crashes) {
    ck_assert_double_eq(candidate_fitness(100, 0, 5), 100);
    ck_assert_double_eq(candidate_fitness(100, 4, 5), 80);
    ck_assert_double_eq(candidate_fitness(100, 4, 0), 100);
    ck_assert_double_eq(candidate_fitness(10.5, 2, 1.5), 7.5);
    ck_assert(candidate_fitness(100, 4, 6) < candidate_fitness(100, 4, 5));
}
END_TEST

START_TEST(test_candidate_same_genes_and_generation_give_the_same_result) {
    double genome[GENOME_LENGTH];
    rng_t rng;
    rng_init(&rng, 1);
    random_genome(&rng, genome);

    srand(1);
    candidate_result_t first = play(genome, 2);
    srand(999); /* whatever rand() was doing before must not matter */
    candidate_result_t second = play(genome, 2);

    ck_assert_double_eq(first.fitness, second.fitness);
    ck_assert_double_eq(first.cars_passed, second.cars_passed);
    ck_assert_double_eq(first.crashes, second.crashes);
}
END_TEST

START_TEST(test_candidate_traffic_changes_between_generations) {
    double genome[GENOME_LENGTH];
    reasonable_genome(genome);

    bool differs = false;
    double first = play(genome, 0).fitness;
    for (unsigned int generation = 1; generation < 6; generation++) {
        differs = differs || play(genome, generation).fitness != first;
    }

    ck_assert(differs);
}
END_TEST

START_TEST(test_candidate_fitness_is_the_mean_of_episodes_with_consecutive_seeds) {
    /* Generation 2 of a 3 episode run plays the seeds 100 + 2 * 3 + {0, 1, 2}. */
    double genome[GENOME_LENGTH];
    rng_t rng;
    rng_init(&rng, 2);
    random_genome(&rng, genome);
    candidate_result_t averaged = play(genome, 2);

    double cars = 0.0;
    for (unsigned int e = 0; e < 3; e++) {
        teardown();
        make_config(1, 300, 5.0, 100 + 2 * 3 + e);
        cars += play(genome, 0).cars_passed;
    }

    ck_assert_double_eq_tol(averaged.cars_passed, cars / 3, 1e-9);
}
END_TEST

START_TEST(test_candidate_a_reasonable_genome_beats_one_that_never_switches) {
    double reasonable[GENOME_LENGTH], never[GENOME_LENGTH];
    reasonable_genome(reasonable);
    never_switching_genome(never);

    for (unsigned int generation = 0; generation < 3; generation++) {
        /* With one road stuck on red, only one road's cars ever pass. */
        ck_assert_double_gt(play(reasonable, generation).fitness, play(never, generation).fitness + 50);
    }
}
END_TEST

START_TEST(test_candidate_linear_combination_never_crashes) {
    /*
     * The agent always shows exactly one green light, and a car that has passed its light is out of
     * the way on the next tick, so the roads never fill the crossing together. If a change to the agent
     * or the game makes this false, the crash penalty starts to matter: look at it again then.
     */
    rng_t rng;
    rng_init(&rng, 3);
    for (int i = 0; i < 20; i++) {
        double genome[GENOME_LENGTH];
        random_genome(&rng, genome);
        candidate_result_t result = play(genome, (unsigned int)i);
        ck_assert_double_eq(result.crashes, 0.0);
        ck_assert_double_eq(result.fitness, result.cars_passed);
    }
}
END_TEST

START_TEST(test_candidate_plays_exactly_like_the_same_numbers_in_an_ai_mode_config) {
    /* A genome pasted into config.ini must behave the way it did in training. */
    teardown();
    make_config(1, 400, 5.0, 77);

    double genome[GENOME_LENGTH];
    rng_t rng;
    rng_init(&rng, 4);
    random_genome(&rng, genome);
    candidate_result_t trained = play(genome, 0);

    char ai_path[64];
    snprintf(ai_path, sizeof(ai_path), "/tmp/semaphore_evaluator_ai_XXXXXX");
    int fd = mkstemp(ai_path);
    ck_assert_int_ge(fd, 0);
    close(fd);

    FILE* f = fopen(ai_path, "w");
    ck_assert_ptr_nonnull(f);
    fprintf(
        f,
        "mode=ai\nagent=linear_combination\nbinarization_method=threshold\nthreshold=%.17g\n",
        genome[0]
    );
    fputs("coeficients_h=", f);
    for (int g = 0; g < WORLD_H; g++) {
        fprintf(f, "%s%.17g", g == 0 ? "" : ",", genome[1 + g]);
    }
    fputs("\ncoeficients_v=", f);
    for (int g = 0; g < WORLD_V; g++) {
        fprintf(f, "%s%.17g", g == 0 ? "" : ",", genome[1 + WORLD_H + g]);
    }
    fputs(
        "\ncar_spawn_probability_h=0.5\ncar_spawn_probability_v=0.5\n"
        "road_length_h=10\nroad_length_v=12\nview_width=21\nview_height=21\ntick_ms=200\n",
        f
    );
    fclose(f);

    config_t* ai_config = NULL;
    ck_assert_int_eq(config_create(ai_path, &ai_config), CONFIG_STATUS_OK);
    game_t* game = NULL;
    ai_t* ai = NULL;
    ck_assert_int_eq(game_create(ai_config, &game), GAME_STATUS_OK);
    ck_assert_int_eq(ai_create(ai_config, &ai), AI_STATUS_OK);

    srand(77);
    for (int tick = 0; tick < 400; tick++) {
        game_exec(game, ai_decide(ai, game));
    }

    ck_assert_double_eq(trained.cars_passed, game_get_cars_passed(game));
    ck_assert_double_eq(trained.crashes, game_get_crashes(game));
    ck_assert_double_gt(trained.cars_passed, 0);

    ai_destroy(&ai);
    game_destroy(&game);
    config_destroy(&ai_config);
    unlink(ai_path);
}
END_TEST

START_TEST(test_candidate_rejects_null_arguments_and_leaves_the_result_alone) {
    double genome[GENOME_LENGTH];
    reasonable_genome(genome);
    candidate_result_t result = { .fitness = 1, .cars_passed = 2, .crashes = 3 };

    ck_assert_int_eq(candidate_play(NULL, genome, 0, &result), CANDIDATE_STATUS_EINVAL);
    ck_assert_int_eq(candidate_play(config, NULL, 0, &result), CANDIDATE_STATUS_EINVAL);
    ck_assert_int_eq(candidate_play(config, genome, 0, NULL), CANDIDATE_STATUS_EINVAL);
    ck_assert_double_eq(result.fitness, 1);
    ck_assert_double_eq(result.cars_passed, 2);
    ck_assert_double_eq(result.crashes, 3);
}
END_TEST

/* ---------------------------------------------------------------- evaluator */

static void random_population(double* genomes, unsigned int count, uint64_t seed) {
    rng_t rng;
    rng_init(&rng, seed);
    for (unsigned int i = 0; i < count; i++) {
        random_genome(&rng, &genomes[(size_t)i * GENOME_LENGTH]);
    }
}

START_TEST(test_evaluator_scores_every_candidate_like_candidate_play) {
    double genomes[POPULATION * GENOME_LENGTH], fitness[POPULATION];
    random_population(genomes, POPULATION, 5);

    evaluator_t* evaluator = NULL;
    ck_assert_int_eq(evaluator_create(config, POPULATION, &evaluator), EVALUATOR_STATUS_OK);
    ck_assert_ptr_null(evaluator_result(evaluator, 0)); /* nothing scored yet */

    ck_assert_int_eq(evaluator_evaluate(genomes, POPULATION, GENOME_LENGTH, 1, fitness, evaluator), 0);

    for (unsigned int i = 0; i < POPULATION; i++) {
        candidate_result_t expected = play(&genomes[i * GENOME_LENGTH], 1);
        const candidate_result_t* kept = evaluator_result(evaluator, i);
        ck_assert_ptr_nonnull(kept);
        ck_assert_double_eq(fitness[i], expected.fitness);
        ck_assert_double_eq(kept->fitness, expected.fitness);
        ck_assert_double_eq(kept->cars_passed, expected.cars_passed);
        ck_assert_double_eq(kept->crashes, expected.crashes);
    }
    ck_assert_ptr_null(evaluator_result(evaluator, POPULATION));

    evaluator_destroy(&evaluator);
    ck_assert_ptr_null(evaluator);
}
END_TEST

START_TEST(test_evaluator_result_does_not_depend_on_the_order_of_the_candidates) {
    /* This is what lets the parallel version split the population between processes. */
    double genomes[POPULATION * GENOME_LENGTH], reversed[POPULATION * GENOME_LENGTH];
    double fitness[POPULATION], reversed_fitness[POPULATION];
    random_population(genomes, POPULATION, 6);
    for (unsigned int i = 0; i < POPULATION; i++) {
        memcpy(
            &reversed[i * GENOME_LENGTH],
            &genomes[(POPULATION - 1 - i) * GENOME_LENGTH],
            GENOME_LENGTH * sizeof(double)
        );
    }

    evaluator_t* evaluator = NULL;
    ck_assert_int_eq(evaluator_create(config, POPULATION, &evaluator), EVALUATOR_STATUS_OK);
    ck_assert_int_eq(evaluator_evaluate(genomes, POPULATION, GENOME_LENGTH, 0, fitness, evaluator), 0);
    ck_assert_int_eq(evaluator_evaluate(reversed, POPULATION, GENOME_LENGTH, 0, reversed_fitness, evaluator), 0);

    for (unsigned int i = 0; i < POPULATION; i++) {
        ck_assert_double_eq(fitness[i], reversed_fitness[POPULATION - 1 - i]);
    }

    evaluator_destroy(&evaluator);
}
END_TEST

START_TEST(test_evaluator_scores_a_smaller_population_than_its_capacity) {
    double genomes[POPULATION * GENOME_LENGTH], fitness[POPULATION];
    random_population(genomes, POPULATION, 7);

    evaluator_t* evaluator = NULL;
    ck_assert_int_eq(evaluator_create(config, POPULATION, &evaluator), EVALUATOR_STATUS_OK);
    ck_assert_int_eq(evaluator_evaluate(genomes, POPULATION, GENOME_LENGTH, 0, fitness, evaluator), 0);
    ck_assert_int_eq(evaluator_evaluate(genomes, 2, GENOME_LENGTH, 0, fitness, evaluator), 0);

    ck_assert_ptr_nonnull(evaluator_result(evaluator, 1));
    ck_assert_ptr_null(evaluator_result(evaluator, 2)); /* only 2 were scored the last time */

    evaluator_destroy(&evaluator);
}
END_TEST

START_TEST(test_evaluator_rejects_a_shape_that_does_not_fit) {
    double genomes[POPULATION * GENOME_LENGTH], fitness[POPULATION];
    random_population(genomes, POPULATION, 8);

    evaluator_t* evaluator = NULL;
    ck_assert_int_eq(evaluator_create(config, POPULATION, &evaluator), EVALUATOR_STATUS_OK);

    ck_assert_int_ne(evaluator_evaluate(genomes, POPULATION, GENOME_LENGTH - 1, 0, fitness, evaluator), 0);
    ck_assert_int_ne(evaluator_evaluate(genomes, POPULATION, GENOME_LENGTH + 1, 0, fitness, evaluator), 0);
    ck_assert_int_ne(evaluator_evaluate(genomes, POPULATION + 1, GENOME_LENGTH, 0, fitness, evaluator), 0);
    ck_assert_int_ne(evaluator_evaluate(NULL, POPULATION, GENOME_LENGTH, 0, fitness, evaluator), 0);
    ck_assert_int_ne(evaluator_evaluate(genomes, POPULATION, GENOME_LENGTH, 0, NULL, evaluator), 0);
    ck_assert_int_ne(evaluator_evaluate(genomes, POPULATION, GENOME_LENGTH, 0, fitness, NULL), 0);

    evaluator_destroy(&evaluator);
}
END_TEST

START_TEST(test_evaluator_create_and_destroy_edge_cases) {
    evaluator_t* evaluator = NULL;

    ck_assert_int_eq(evaluator_create(NULL, POPULATION, &evaluator), EVALUATOR_STATUS_EINVAL);
    ck_assert_int_eq(evaluator_create(config, 0, &evaluator), EVALUATOR_STATUS_EINVAL);
    ck_assert_int_eq(evaluator_create(config, POPULATION, NULL), EVALUATOR_STATUS_EINVAL);
    ck_assert_ptr_null(evaluator);

    evaluator_destroy(&evaluator); /* NULL inside: fine */
    evaluator_destroy(NULL);
    ck_assert_ptr_null(evaluator_result(NULL, 0));
}
END_TEST

typedef struct run_check {
    evaluator_t* evaluator;
    unsigned int reports;
    bool champion_matches;
} run_check_t;

/**
 * genetic_run gives the same context to evaluate and report, so a caller that needs its own context
 * for the report (like training mode) wraps the evaluator, as here.
 */
static int evaluate_with_check(
    const double* genomes,
    unsigned int count,
    unsigned int genome_length,
    unsigned int generation,
    double* fitness_out,
    void* context
) {
    run_check_t* check = context;

    return evaluator_evaluate(genomes, count, genome_length, generation, fitness_out, check->evaluator);
}

static int check_report(const genetic_generation_t* generation, void* context) {
    run_check_t* check = context;
    const candidate_result_t* champion = evaluator_result(check->evaluator, generation->best_index);

    check->reports++;
    check->champion_matches = champion != NULL && champion->fitness == generation->best_fitness;
    return check->champion_matches ? 0 : 1;
}

START_TEST(test_evaluator_drives_a_genetic_run) {
    /* The way training mode will use it: evaluator_evaluate called from the evaluate callback. */
    teardown();
    make_config(1, 100, 5.0, 11);

    double min[GENOME_LENGTH], max[GENOME_LENGTH], best[GENOME_LENGTH];
    candidate_genome_ranges(config, min, max);

    run_check_t check = { .evaluator = NULL, .reports = 0, .champion_matches = false };
    ck_assert_int_eq(evaluator_create(config, POPULATION, &check.evaluator), EVALUATOR_STATUS_OK);

    genetic_params_t params = {
        .population_size = POPULATION,
        .genome_length = GENOME_LENGTH,
        .generations = 3,
        .elite_count = 1,
        .tournament_size = 2,
        .mutation_rate = 0.2,
        .mutation_sigma = 0.2,
        .seed = 11,
    };
    genetic_result_t result;

    ck_assert_int_eq(
        genetic_run(&params, min, max, evaluate_with_check, check_report, &check, &result, best),
        GENETIC_STATUS_OK
    );
    ck_assert_uint_eq(check.reports, 3);
    ck_assert(isfinite(result.best_fitness));
    for (int g = 0; g < GENOME_LENGTH; g++) {
        ck_assert_double_ge(best[g], min[g]);
        ck_assert_double_le(best[g], max[g]);
    }

    evaluator_destroy(&check.evaluator);
}
END_TEST

Suite* evaluator_suite(void) {
    Suite* s = suite_create("evaluator");
    TCase* tc = tcase_create("core");
    tcase_set_timeout(tc, 30);
    tcase_add_checked_fixture(tc, setup, teardown);

    tcase_add_test(tc, test_candidate_genome_has_a_threshold_and_one_gene_per_road_cell);
    tcase_add_test(tc, test_candidate_ranges_follow_the_config);
    tcase_add_test(tc, test_candidate_fitness_is_cars_minus_penalized_crashes);
    tcase_add_test(tc, test_candidate_same_genes_and_generation_give_the_same_result);
    tcase_add_test(tc, test_candidate_traffic_changes_between_generations);
    tcase_add_test(tc, test_candidate_fitness_is_the_mean_of_episodes_with_consecutive_seeds);
    tcase_add_test(tc, test_candidate_a_reasonable_genome_beats_one_that_never_switches);
    tcase_add_test(tc, test_candidate_linear_combination_never_crashes);
    tcase_add_test(tc, test_candidate_plays_exactly_like_the_same_numbers_in_an_ai_mode_config);
    tcase_add_test(tc, test_candidate_rejects_null_arguments_and_leaves_the_result_alone);

    tcase_add_test(tc, test_evaluator_scores_every_candidate_like_candidate_play);
    tcase_add_test(tc, test_evaluator_result_does_not_depend_on_the_order_of_the_candidates);
    tcase_add_test(tc, test_evaluator_scores_a_smaller_population_than_its_capacity);
    tcase_add_test(tc, test_evaluator_rejects_a_shape_that_does_not_fit);
    tcase_add_test(tc, test_evaluator_create_and_destroy_edge_cases);
    tcase_add_test(tc, test_evaluator_drives_a_genetic_run);

    suite_add_tcase(s, tc);
    return s;
}
