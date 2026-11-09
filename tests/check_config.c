#define _POSIX_C_SOURCE 200809L /* mkstemp is hidden under strict -std=c2x */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/config/config.h"
#include "suites.h"

static char path[64];
static config_t* config;

/** Writes `contents` to a fresh temp file and returns its path. */
static const char* write_config(const char* contents) {
    FILE* f = fopen(path, "w");
    ck_assert_ptr_nonnull(f);
    fputs(contents, f);
    fclose(f);
    return path;
}

/** A complete human-mode config. Tests that need a variation start from the pieces. */
#define MODE_HUMAN "mode=human\n"
#define SPAWN_H "car_spawn_probability_h=0.25\n"
#define SPAWN_V "car_spawn_probability_v=0.75\n"

/** Writes `contents` to the temp file and builds `config` from it. */
static config_status_t create_from(const char* contents) {
    return config_create(write_config(contents), &config);
}

static void setup(void) {
    /* mkstemp rewrites the template in place, so rebuild it for every test (CK_FORK=no shares it). */
    snprintf(path, sizeof(path), "/tmp/semaphore_config_XXXXXX");
    int fd = mkstemp(path);
    ck_assert_int_ge(fd, 0);
    close(fd);

    config = NULL;
}

static void teardown(void) {
    config_destroy(&config); /* safe when creation failed or a test already destroyed it */
    unlink(path);
}

START_TEST(test_config_parses_human_mode) {
    ck_assert_int_eq(create_from(
        "[game]\n"
        MODE_HUMAN
        "\n"
        "[human_player]\n"
        SPAWN_H
        SPAWN_V), CONFIG_STATUS_OK);

    ck_assert_int_eq(config_get_mode(config), MODE_HUMAN_PLAYER);
    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_v, 0.75, 1e-9);
}
END_TEST

START_TEST(test_config_parses_ai_mode) {
    ck_assert_int_eq(create_from("mode=ai\n"), CONFIG_STATUS_OK);
    ck_assert_int_eq(config_get_mode(config), MODE_AI_PLAYER);
}
END_TEST

START_TEST(test_config_parses_training_mode) {
    ck_assert_int_eq(create_from("mode=training\n"), CONFIG_STATUS_OK);
    ck_assert_int_eq(config_get_mode(config), MODE_TRAINING);
}
END_TEST

START_TEST(test_config_key_order_does_not_matter) {
    ck_assert_int_eq(create_from(SPAWN_V SPAWN_H MODE_HUMAN), CONFIG_STATUS_OK);

    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_v, 0.75, 1e-9);
}
END_TEST

START_TEST(test_config_skips_comments_and_blank_lines) {
    ck_assert_int_eq(create_from(
        "# hash comment\n"
        "; semicolon comment\n"
        "\n"
        MODE_HUMAN
        SPAWN_H
        SPAWN_V), CONFIG_STATUS_OK);

    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
}
END_TEST

START_TEST(test_config_ignores_unknown_keys) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V "something_else=1\n"), CONFIG_STATUS_OK);
}
END_TEST

START_TEST(test_config_accepts_trailing_whitespace) {
    ck_assert_int_eq(create_from(MODE_HUMAN "car_spawn_probability_h=0.25 \r\n" SPAWN_V), CONFIG_STATUS_OK);
    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
}
END_TEST

START_TEST(test_config_empty_file_is_missing_mode) {
    ck_assert_int_eq(create_from(""), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_missing_mode) {
    ck_assert_int_eq(create_from(SPAWN_H SPAWN_V), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_spawn_h) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_V), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_spawn_v) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_other_modes_do_not_need_human_keys) {
    /* Only the keys of the chosen mode are required. */
    ck_assert_int_eq(create_from("mode=ai\n"), CONFIG_STATUS_OK);
}
END_TEST

START_TEST(test_config_unknown_mode) {
    ck_assert_int_eq(create_from("mode=banana\n" SPAWN_H SPAWN_V), CONFIG_STATUS_EPARSE);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_invalid_probability) {
    ck_assert_int_eq(create_from(MODE_HUMAN "car_spawn_probability_h=abc\n" SPAWN_V), CONFIG_STATUS_EPARSE);
    ck_assert_ptr_null(config);

    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H "car_spawn_probability_v=0.5x\n"), CONFIG_STATUS_EPARSE);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_nan_probability_is_invalid) {
    /* NaN is what marks an unset field internally, so it must not be accepted as a value. */
    ck_assert_int_eq(create_from(MODE_HUMAN "car_spawn_probability_h=nan\n" SPAWN_V), CONFIG_STATUS_EPARSE);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_malformed_line) {
    ck_assert_int_eq(create_from(MODE_HUMAN "this line has no equals sign\n"), CONFIG_STATUS_EPARSE);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_missing_file) {
    ck_assert_int_eq(config_create("/nonexistent/config.ini", &config), CONFIG_STATUS_EACCESS);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_getter_returns_copy) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V), CONFIG_STATUS_OK);

    config_human_player_t copy = config_get_human_player(config);
    copy.car_spawn_probability_h = 0.99;

    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
}
END_TEST

START_TEST(test_config_getter_outlives_config) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V), CONFIG_STATUS_OK);

    config_human_player_t copy = config_get_human_player(config);
    config_destroy(&config);

    ck_assert_double_eq_tol(copy.car_spawn_probability_v, 0.75, 1e-9);
}
END_TEST

START_TEST(test_config_destroy_nulls_pointer) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V), CONFIG_STATUS_OK);

    config_destroy(&config);

    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_destroy_null_safe) {
    config_t* none = NULL;

    config_destroy(&none);
    config_destroy(NULL);
}
END_TEST

START_TEST(test_config_shipped_config_ini_parses) {
    /* Guards the config.ini that main() actually loads. `make test` runs from the repo root. */
    ck_assert_int_eq(config_create("config.ini", &config), CONFIG_STATUS_OK);
    ck_assert_int_eq(config_get_mode(config), MODE_HUMAN_PLAYER);
}
END_TEST

Suite* config_suite(void) {
    Suite* s = suite_create("config");
    TCase* tc = tcase_create("core");

    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_config_parses_human_mode);
    tcase_add_test(tc, test_config_parses_ai_mode);
    tcase_add_test(tc, test_config_parses_training_mode);
    tcase_add_test(tc, test_config_key_order_does_not_matter);
    tcase_add_test(tc, test_config_skips_comments_and_blank_lines);
    tcase_add_test(tc, test_config_ignores_unknown_keys);
    tcase_add_test(tc, test_config_accepts_trailing_whitespace);
    tcase_add_test(tc, test_config_empty_file_is_missing_mode);
    tcase_add_test(tc, test_config_missing_mode);
    tcase_add_test(tc, test_config_human_mode_missing_spawn_h);
    tcase_add_test(tc, test_config_human_mode_missing_spawn_v);
    tcase_add_test(tc, test_config_other_modes_do_not_need_human_keys);
    tcase_add_test(tc, test_config_unknown_mode);
    tcase_add_test(tc, test_config_invalid_probability);
    tcase_add_test(tc, test_config_nan_probability_is_invalid);
    tcase_add_test(tc, test_config_malformed_line);
    tcase_add_test(tc, test_config_missing_file);
    tcase_add_test(tc, test_config_getter_returns_copy);
    tcase_add_test(tc, test_config_getter_outlives_config);
    tcase_add_test(tc, test_config_destroy_nulls_pointer);
    tcase_add_test(tc, test_config_destroy_null_safe);
    tcase_add_test(tc, test_config_shipped_config_ini_parses);

    suite_add_tcase(s, tc);
    return s;
}
