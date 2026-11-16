#include <check.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../src/training/genetic.h"
#include "suites.h"

/**
 * The engine is tested with plain math functions and no game: the fitness is -sum(x^2), whose best
 * genome is all zeros, so "it got better" is easy to see.
 */

#define POPULATION 30
#define LENGTH 6
#define MAX_GENERATIONS 64

static const double MIN[LENGTH] = { -5, -5, -5, -5, -5, -5 };
static const double MAX[LENGTH] = { 5, 5, 5, 5, 5, 5 };

typedef struct recorder {
    /** Where the genes must stay. */
    const double* min;
    const double* max;
    unsigned int length;

    /** What the callbacks were asked to do. */
    int fail_on_generation;     // evaluate returns 1 on this generation (-1: never)
    int nan_on_generation;      // evaluate gives a NaN fitness on this generation (-1: never)
    unsigned int stop_after;    // report returns 1 after this many reports (0: never)
    double generation_penalty;  // fitness loses this much per generation
    bool constant_fitness;      // every candidate scores 0
    bool check_elites;          // the previous champion must be genome 0 of the next population
    bool check_inheritance;     // every gene of a child must exist at that position in the previous population

    /** What they saw. */
    unsigned int evaluations;
    unsigned int reports;
    double champion_fitness[MAX_GENERATIONS];
    double champion_genome[MAX_GENERATIONS][LENGTH];
    double mean_fitness[MAX_GENERATIONS];
    double previous[POPULATION * LENGTH];
    double previous_champion[LENGTH];
} recorder_t;

static recorder_t new_recorder(void) {
    recorder_t r;
    memset(&r, 0, sizeof(r));
    r.min = MIN;
    r.max = MAX;
    r.length = LENGTH;
    r.fail_on_generation = -1;
    r.nan_on_generation = -1;
    return r;
}

static genetic_params_t new_params(void) {
    return (genetic_params_t) {
        .population_size = POPULATION,
        .genome_length = LENGTH,
        .generations = 40,
        .elite_count = 2,
        .tournament_size = 3,
        .mutation_rate = 0.2,
        .mutation_sigma = 0.1,
        .seed = 42,
    };
}

static int evaluate(const double* genomes, unsigned int count, unsigned int length, unsigned int generation, double* fitness_out, void* context) {
    recorder_t* r = context;

    ck_assert_uint_eq(generation, r->evaluations);
    ck_assert_uint_eq(count, POPULATION);
    ck_assert_uint_eq(length, r->length);

    for (unsigned int i = 0; i < count; i++) {
        double sum = 0.0;
        for (unsigned int g = 0; g < length; g++) {
            double gene = genomes[i * length + g];
            ck_assert_msg(gene >= r->min[g] && gene <= r->max[g], "gene %u of candidate %u is %f, outside [%f, %f]", g, i, gene, r->min[g], r->max[g]);
            sum += gene * gene;
        }

        fitness_out[i] = r->constant_fitness ? 0.0 : -sum - r->generation_penalty * generation;
    }

    if (generation > 0 && r->check_elites) {
        ck_assert_msg(memcmp(genomes, r->previous_champion, length * sizeof(double)) == 0, "the champion of generation %u is not first in generation %u", generation - 1, generation);
    }

    if (generation > 0 && r->check_inheritance) {
        for (unsigned int i = 0; i < count; i++) {
            for (unsigned int g = 0; g < length; g++) {
                bool found = false;
                for (unsigned int j = 0; j < count; j++) {
                    found = found || genomes[i * length + g] == r->previous[j * length + g];
                }

                ck_assert_msg(found, "gene %u of candidate %u was not in the previous population", g, i);
            }
        }
    }

    memcpy(r->previous, genomes, count * length * sizeof(double));

    if (r->nan_on_generation == (int)generation) {
        fitness_out[0] = NAN;
    }

    if (r->fail_on_generation == (int)generation) {
        return 1;
    }

    r->evaluations++;
    return 0;
}

static int report(const genetic_generation_t* generation, void* context) {
    recorder_t* r = context;
    unsigned int index = generation->index;
    ck_assert_uint_eq(index, r->reports);

    /** The summary has to agree with the data it points at. */
    double best = generation->fitness[0];
    double worst = generation->fitness[0];
    double sum = 0.0;
    for (unsigned int i = 0; i < POPULATION; i++) {
        best = fmax(best, generation->fitness[i]);
        worst = fmin(worst, generation->fitness[i]);
        sum += generation->fitness[i];
    }

    ck_assert_double_eq(generation->best_fitness, best);
    ck_assert_double_eq(generation->worst_fitness, worst);
    ck_assert_double_eq_tol(generation->mean_fitness, sum / POPULATION, 1e-9);
    ck_assert_double_eq(generation->fitness[generation->best_index], best);
    ck_assert_ptr_eq(generation->best_genome, &generation->genomes[generation->best_index * r->length]);

    r->champion_fitness[index] = generation->best_fitness;
    r->mean_fitness[index] = generation->mean_fitness;
    memcpy(r->champion_genome[index], generation->best_genome, r->length * sizeof(double));
    memcpy(r->previous_champion, generation->best_genome, r->length * sizeof(double));

    r->reports++;
    return r->stop_after != 0 && r->reports >= r->stop_after ? 1 : 0;
}

START_TEST(test_genetic_improves_the_population) {
    recorder_t r = new_recorder();
    genetic_params_t params = new_params();
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_OK);

    ck_assert_uint_eq(r.evaluations, 40);
    ck_assert_uint_eq(r.reports, 40);
    /* The random start is about -50 on average; 40 generations of selection must get far above it. */
    ck_assert_double_gt(r.champion_fitness[39], r.champion_fitness[0]);
    ck_assert_double_gt(r.champion_fitness[39], r.mean_fitness[0] + 20.0);
    ck_assert_double_gt(r.champion_fitness[39], -5.0);
    ck_assert_double_gt(r.mean_fitness[39], r.mean_fitness[0]);
}
END_TEST

START_TEST(test_genetic_champion_never_gets_worse_with_elites) {
    recorder_t r = new_recorder();
    r.check_elites = true; /* also checks, in evaluate, that the champion is copied into slot 0 */
    genetic_params_t params = new_params();
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_OK);

    for (unsigned int g = 1; g < 40; g++) {
        ck_assert_double_ge(r.champion_fitness[g], r.champion_fitness[g - 1]);
    }
}
END_TEST

START_TEST(test_genetic_result_is_the_best_seen) {
    recorder_t r = new_recorder();
    genetic_params_t params = new_params();
    genetic_result_t result;
    double best[LENGTH];

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, best), GENETIC_STATUS_OK);

    double expected = r.champion_fitness[0];
    unsigned int expected_generation = 0;
    for (unsigned int g = 1; g < 40; g++) {
        if (r.champion_fitness[g] > expected) {
            expected = r.champion_fitness[g];
            expected_generation = g;
        }
    }

    ck_assert_double_eq(result.best_fitness, expected);
    ck_assert_uint_eq(result.best_generation, expected_generation);
    ck_assert_int_eq(memcmp(best, r.champion_genome[expected_generation], sizeof(best)), 0);
}
END_TEST

START_TEST(test_genetic_best_seen_can_come_from_an_early_generation) {
    /* Every generation scores 1000 lower than the one before, so generation 0 holds the best. */
    recorder_t r = new_recorder();
    r.generation_penalty = 1000.0;
    genetic_params_t params = new_params();
    genetic_result_t result;
    double best[LENGTH];

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, best), GENETIC_STATUS_OK);

    ck_assert_uint_eq(result.best_generation, 0);
    ck_assert_double_eq(result.best_fitness, r.champion_fitness[0]);
    ck_assert_int_eq(memcmp(best, r.champion_genome[0], sizeof(best)), 0);
}
END_TEST

START_TEST(test_genetic_same_seed_gives_the_same_run) {
    recorder_t a = new_recorder();
    recorder_t b = new_recorder();
    genetic_params_t params = new_params();
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &a, &result, NULL), GENETIC_STATUS_OK);
    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &b, &result, NULL), GENETIC_STATUS_OK);

    ck_assert_int_eq(memcmp(a.champion_fitness, b.champion_fitness, sizeof(a.champion_fitness)), 0);
    ck_assert_int_eq(memcmp(a.champion_genome, b.champion_genome, sizeof(a.champion_genome)), 0);
}
END_TEST

START_TEST(test_genetic_a_different_seed_gives_a_different_run) {
    recorder_t a = new_recorder();
    recorder_t b = new_recorder();
    genetic_params_t params = new_params();
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &a, &result, NULL), GENETIC_STATUS_OK);
    params.seed = 43;
    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &b, &result, NULL), GENETIC_STATUS_OK);

    ck_assert_int_ne(memcmp(a.champion_genome, b.champion_genome, sizeof(a.champion_genome)), 0);
}
END_TEST

START_TEST(test_genetic_does_not_use_rand) {
    /* Reseeding the global generator between runs, as the game evaluator does, changes nothing. */
    recorder_t a = new_recorder();
    recorder_t b = new_recorder();
    genetic_params_t params = new_params();
    genetic_result_t result;

    srand(1);
    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &a, &result, NULL), GENETIC_STATUS_OK);
    srand(999);
    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &b, &result, NULL), GENETIC_STATUS_OK);

    ck_assert_int_eq(memcmp(a.champion_genome, b.champion_genome, sizeof(a.champion_genome)), 0);
}
END_TEST

START_TEST(test_genetic_genes_stay_inside_their_own_range) {
    /* Different ranges, and one gene that cannot move at all. evaluate checks every gene of every generation. */
    static const double min[3] = { 0.0, -3.0, 7.0 };
    static const double max[3] = { 1.0, -2.0, 7.0 };
    recorder_t r = new_recorder();
    r.min = min;
    r.max = max;
    r.length = 3;
    genetic_params_t params = new_params();
    params.genome_length = 3;
    params.mutation_rate = 1.0; /* every gene mutates, with big steps, to push against the limits */
    params.mutation_sigma = 5.0;
    genetic_result_t result;
    double best[3];

    ck_assert_int_eq(genetic_run(&params, min, max, evaluate, report, &r, &result, best), GENETIC_STATUS_OK);
    ck_assert_double_eq(best[2], 7.0);
}
END_TEST

START_TEST(test_genetic_without_mutation_children_only_mix_existing_genes) {
    recorder_t r = new_recorder();
    r.check_inheritance = true;
    genetic_params_t params = new_params();
    params.mutation_rate = 0.0;
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_OK);
}
END_TEST

START_TEST(test_genetic_single_generation_evaluates_once) {
    recorder_t r = new_recorder();
    genetic_params_t params = new_params();
    params.generations = 1;
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_OK);
    ck_assert_uint_eq(r.evaluations, 1);
    ck_assert_uint_eq(r.reports, 1);
    ck_assert_uint_eq(result.best_generation, 0);
    ck_assert_double_eq(result.best_fitness, r.champion_fitness[0]);
}
END_TEST

static int sum_evaluate(const double* genomes, unsigned int count, unsigned int length, unsigned int generation, double* fitness_out, void* context) {
    (void) generation;
    (void) context;
    for (unsigned int i = 0; i < count; i++) {
        double sum = 0.0;
        for (unsigned int g = 0; g < length; g++) {
            sum += genomes[i * length + g] * genomes[i * length + g];
        }
        fitness_out[i] = -sum;
    }
    return 0;
}

START_TEST(test_genetic_odd_population_shapes) {
    genetic_result_t result;
    genetic_params_t params = new_params();

    /* The smallest population that can select. */
    params.population_size = 2;
    params.elite_count = 1;
    params.tournament_size = 2;
    ck_assert_int_eq(genetic_run(&params, MIN, MAX, sum_evaluate, NULL, NULL, &result, NULL), GENETIC_STATUS_OK);

    /* No elites at all, and a tournament of one (selection is then pure luck). */
    params = new_params();
    params.elite_count = 0;
    params.tournament_size = 1;
    ck_assert_int_eq(genetic_run(&params, MIN, MAX, sum_evaluate, NULL, NULL, &result, NULL), GENETIC_STATUS_OK);

    /* Everything but one slot is an elite. */
    params = new_params();
    params.elite_count = POPULATION - 1;
    params.tournament_size = POPULATION;
    ck_assert_int_eq(genetic_run(&params, MIN, MAX, sum_evaluate, NULL, NULL, &result, NULL), GENETIC_STATUS_OK);

    /* A genome of one number. */
    params = new_params();
    params.genome_length = 1;
    ck_assert_int_eq(genetic_run(&params, MIN, MAX, sum_evaluate, NULL, NULL, &result, NULL), GENETIC_STATUS_OK);
}
END_TEST

START_TEST(test_genetic_ties_go_to_the_lowest_index) {
    recorder_t r = new_recorder();
    r.constant_fitness = true;
    genetic_params_t params = new_params();
    params.generations = 3;
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_OK);
    ck_assert_double_eq(result.best_fitness, 0.0);
    ck_assert_uint_eq(result.best_generation, 0); /* the first one on a tie */
}
END_TEST

START_TEST(test_genetic_works_without_report_and_without_best_genome) {
    genetic_result_t result;
    genetic_params_t params = new_params();

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, sum_evaluate, NULL, NULL, &result, NULL), GENETIC_STATUS_OK);
}
END_TEST

START_TEST(test_genetic_rejects_invalid_params) {
    genetic_result_t result;
    recorder_t r = new_recorder();
    genetic_params_t good = new_params();
    genetic_params_t bad[12];

    for (int i = 0; i < 12; i++) bad[i] = good;
    bad[0].population_size = 1;
    bad[1].genome_length = 0;
    bad[2].generations = 0;
    bad[3].elite_count = POPULATION;
    bad[4].tournament_size = 0;
    bad[5].tournament_size = POPULATION + 1;
    bad[6].mutation_rate = -0.1;
    bad[7].mutation_rate = 1.1;
    bad[8].mutation_rate = NAN;
    bad[9].mutation_sigma = 0.0;
    bad[10].mutation_sigma = INFINITY;
    bad[11].mutation_sigma = NAN;

    for (int i = 0; i < 12; i++) {
        ck_assert_msg(genetic_run(&bad[i], MIN, MAX, evaluate, report, &r, &result, NULL) == GENETIC_STATUS_EINVAL, "params %d were accepted", i);
    }

    /* Ranges that are backwards or not finite, and missing pieces. */
    double backwards[LENGTH] = { 6, -5, -5, -5, -5, -5 };
    double infinite[LENGTH] = { -5, -5, -5, -5, -5, INFINITY };
    ck_assert_int_eq(genetic_run(&good, backwards, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_EINVAL);
    ck_assert_int_eq(genetic_run(&good, MIN, infinite, evaluate, report, &r, &result, NULL), GENETIC_STATUS_EINVAL);
    ck_assert_int_eq(genetic_run(NULL, MIN, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_EINVAL);
    ck_assert_int_eq(genetic_run(&good, NULL, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_EINVAL);
    ck_assert_int_eq(genetic_run(&good, MIN, NULL, evaluate, report, &r, &result, NULL), GENETIC_STATUS_EINVAL);
    ck_assert_int_eq(genetic_run(&good, MIN, MAX, NULL, report, &r, &result, NULL), GENETIC_STATUS_EINVAL);
    ck_assert_int_eq(genetic_run(&good, MIN, MAX, evaluate, report, &r, NULL, NULL), GENETIC_STATUS_EINVAL);

    ck_assert_uint_eq(r.evaluations, 0); /* nothing ran */
}
END_TEST

START_TEST(test_genetic_stops_when_evaluate_fails) {
    recorder_t r = new_recorder();
    r.fail_on_generation = 5;
    genetic_params_t params = new_params();
    genetic_result_t result = { .best_fitness = 123.0, .best_generation = 77 };
    double best[LENGTH] = { 9, 9, 9, 9, 9, 9 };

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, best), GENETIC_STATUS_EEVALUATE);

    ck_assert_uint_eq(r.evaluations, 5);
    ck_assert_uint_eq(r.reports, 5); /* the failed generation is not reported */
    ck_assert_double_eq(result.best_fitness, 123.0); /* untouched on failure */
    ck_assert_uint_eq(result.best_generation, 77);
    ck_assert_double_eq(best[0], 9.0);
}
END_TEST

START_TEST(test_genetic_a_nan_fitness_is_an_evaluation_error) {
    recorder_t r = new_recorder();
    r.nan_on_generation = 2;
    genetic_params_t params = new_params();
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_EEVALUATE);
    ck_assert_uint_eq(r.reports, 2);
}
END_TEST

START_TEST(test_genetic_stops_when_report_asks_to) {
    recorder_t r = new_recorder();
    r.stop_after = 3;
    genetic_params_t params = new_params();
    genetic_result_t result;

    ck_assert_int_eq(genetic_run(&params, MIN, MAX, evaluate, report, &r, &result, NULL), GENETIC_STATUS_EREPORT);
    ck_assert_uint_eq(r.reports, 3);
    ck_assert_uint_eq(r.evaluations, 3);
}
END_TEST

Suite* genetic_suite(void) {
    Suite* s = suite_create("genetic");
    TCase* tc = tcase_create("core");

    tcase_add_test(tc, test_genetic_improves_the_population);
    tcase_add_test(tc, test_genetic_champion_never_gets_worse_with_elites);
    tcase_add_test(tc, test_genetic_result_is_the_best_seen);
    tcase_add_test(tc, test_genetic_best_seen_can_come_from_an_early_generation);
    tcase_add_test(tc, test_genetic_same_seed_gives_the_same_run);
    tcase_add_test(tc, test_genetic_a_different_seed_gives_a_different_run);
    tcase_add_test(tc, test_genetic_does_not_use_rand);
    tcase_add_test(tc, test_genetic_genes_stay_inside_their_own_range);
    tcase_add_test(tc, test_genetic_without_mutation_children_only_mix_existing_genes);
    tcase_add_test(tc, test_genetic_single_generation_evaluates_once);
    tcase_add_test(tc, test_genetic_odd_population_shapes);
    tcase_add_test(tc, test_genetic_ties_go_to_the_lowest_index);
    tcase_add_test(tc, test_genetic_works_without_report_and_without_best_genome);
    tcase_add_test(tc, test_genetic_rejects_invalid_params);
    tcase_add_test(tc, test_genetic_stops_when_evaluate_fails);
    tcase_add_test(tc, test_genetic_a_nan_fitness_is_an_evaluation_error);
    tcase_add_test(tc, test_genetic_stops_when_report_asks_to);

    suite_add_tcase(s, tc);
    return s;
}
