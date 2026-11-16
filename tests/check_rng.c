#include <check.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../src/training/rng.h"
#include "suites.h"

#define SAMPLES 100000

START_TEST(test_rng_matches_the_splitmix64_reference) {
    // First outputs of the reference splitmix64 for seed 0.
    rng_t rng;
    rng_init(&rng, 0);

    ck_assert_uint_eq(rng_next(&rng), 0xE220A8397B1DCDAFULL);
    ck_assert_uint_eq(rng_next(&rng), 0x6E789E6AA1B965F4ULL);
    ck_assert_uint_eq(rng_next(&rng), 0x06C45D188009454FULL);
}
END_TEST

START_TEST(test_rng_same_seed_same_sequence_and_different_seed_differs) {
    rng_t a, b, c;
    rng_init(&a, 42);
    rng_init(&b, 42);
    rng_init(&c, 43);

    bool differs = false;
    for (int i = 0; i < 100; i++) {
        uint64_t va = rng_next(&a);
        ck_assert_uint_eq(va, rng_next(&b));
        differs = differs || va != rng_next(&c);
    }
    ck_assert(differs);
}
END_TEST

START_TEST(test_rng_does_not_touch_rand) {
    srand(7);
    int expected = rand();

    srand(7);
    rng_t rng;
    rng_init(&rng, 7);
    for (int i = 0; i < 50; i++) {
        rng_next(&rng);
        rng_uniform(&rng);
        rng_gaussian(&rng);
    }

    ck_assert_int_eq(rand(), expected);
}
END_TEST

START_TEST(test_rng_uniform_is_in_zero_one_with_mean_half) {
    rng_t rng;
    rng_init(&rng, 1);

    double sum = 0;
    for (int i = 0; i < SAMPLES; i++) {
        double u = rng_uniform(&rng);
        ck_assert(u >= 0.0 && u < 1.0);
        sum += u;
    }

    ck_assert(fabs((sum / SAMPLES) - 0.5) < 0.01);
}
END_TEST

START_TEST(test_rng_below_stays_below_and_reaches_every_value) {
    rng_t rng;
    rng_init(&rng, 2);

    unsigned int seen[7] = { 0 };
    for (int i = 0; i < SAMPLES; i++) {
        unsigned int v = rng_below(&rng, 7);
        ck_assert_uint_lt(v, 7);
        seen[v]++;
    }

    for (int v = 0; v < 7; v++) {
        // Each value should get about 1/7 of the draws.
        ck_assert_uint_gt(seen[v], SAMPLES / 7 * 9 / 10);
        ck_assert_uint_lt(seen[v], SAMPLES / 7 * 11 / 10);
    }

    // n = 1 has only one possible answer.
    ck_assert_uint_eq(rng_below(&rng, 1), 0);
}
END_TEST

START_TEST(test_rng_gaussian_has_mean_zero_and_deviation_one) {
    rng_t rng;
    rng_init(&rng, 3);

    double sum = 0;
    double sum_squares = 0;
    for (int i = 0; i < SAMPLES; i++) {
        double g = rng_gaussian(&rng);
        ck_assert(isfinite(g));
        sum += g;
        sum_squares += g * g;
    }

    double mean = sum / SAMPLES;
    double variance = sum_squares / SAMPLES - mean * mean;
    ck_assert(fabs((mean) - 0.0) < 0.02);
    ck_assert(fabs((variance) - 1.0) < 0.03);
}
END_TEST

Suite* rng_suite(void) {
    Suite* s = suite_create("rng");
    TCase* tc = tcase_create("core");

    tcase_add_test(tc, test_rng_matches_the_splitmix64_reference);
    tcase_add_test(tc, test_rng_same_seed_same_sequence_and_different_seed_differs);
    tcase_add_test(tc, test_rng_does_not_touch_rand);
    tcase_add_test(tc, test_rng_uniform_is_in_zero_one_with_mean_half);
    tcase_add_test(tc, test_rng_below_stays_below_and_reaches_every_value);
    tcase_add_test(tc, test_rng_gaussian_has_mean_zero_and_deviation_one);

    suite_add_tcase(s, tc);
    return s;
}
