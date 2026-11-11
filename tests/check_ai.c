#define _POSIX_C_SOURCE 200809L /* mkstemp is hidden under strict -std=c2x */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/ai/ai.h"
#include "../src/config/config.h"
#include "../src/game/game.h"
#include "suites.h"

static char path[64];
static config_t* config;
static game_t* game;
static ai_t* ai;

static void setup(void) {
    snprintf(path, sizeof(path), "/tmp/semaphore_ai_XXXXXX");
    int fd = mkstemp(path);
    ck_assert_int_ge(fd, 0);
    close(fd);

    FILE* f = fopen(path, "w");
    ck_assert_ptr_nonnull(f);
    fputs(
        "mode=ai\n"
        "car_spawn_probability_h=0.5\n"
        "car_spawn_probability_v=0.5\n"
        "road_length_h=10\n"
        "road_length_v=12\n"
        "view_width=21\n"
        "view_height=21\n"
        "tick_ms=200\n",
        f);
    fclose(f);

    config = NULL;
    game = NULL;
    ai = NULL;
    ck_assert_int_eq(config_create(path, &config), CONFIG_STATUS_OK);
    ck_assert_int_eq(game_create(config, &game), GAME_STATUS_OK);
}

static void teardown(void) {
    ai_destroy(&ai);
    game_destroy(&game);
    config_destroy(&config);
    unlink(path);
}

START_TEST(test_ai_create_gives_an_ai) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
    ck_assert_ptr_nonnull(ai);
}
END_TEST

START_TEST(test_ai_outlives_config) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
    config_destroy(&config);
    ck_assert_int_eq(ai_decide(ai, game), GAME_COMMAND_NOOP);
}
END_TEST

START_TEST(test_ai_always_decides_noop) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);

    for (int tick = 0; tick < 50; tick++) {
        game_command_t command = ai_decide(ai, game);
        ck_assert_int_eq(command, GAME_COMMAND_NOOP);
        game_exec(game, command);
    }
}
END_TEST

START_TEST(test_ai_decide_does_not_change_the_game) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
    unsigned int crashes = game_get_crashes(game);
    game_state_t state = game_get_state(game);

    ai_decide(ai, game);

    ck_assert_uint_eq(game_get_crashes(game), crashes);
    ck_assert_int_eq(game_get_state(game), state);
}
END_TEST

START_TEST(test_ai_destroy_nulls_pointer) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
    ai_destroy(&ai);
    ck_assert_ptr_null(ai);
}
END_TEST

START_TEST(test_ai_destroy_null_safe) {
    ai_t* none = NULL;
    ai_destroy(&none);
    ai_destroy(NULL);
}
END_TEST

Suite* ai_suite(void) {
    Suite* s = suite_create("ai");
    TCase* tc = tcase_create("core");
    tcase_add_checked_fixture(tc, setup, teardown);

    tcase_add_test(tc, test_ai_create_gives_an_ai);
    tcase_add_test(tc, test_ai_outlives_config);
    tcase_add_test(tc, test_ai_always_decides_noop);
    tcase_add_test(tc, test_ai_decide_does_not_change_the_game);
    tcase_add_test(tc, test_ai_destroy_nulls_pointer);
    tcase_add_test(tc, test_ai_destroy_null_safe);

    suite_add_tcase(s, tc);
    return s;
}
