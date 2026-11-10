#define _POSIX_C_SOURCE 200809L /* mkstemp is hidden under strict -std=c2x */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/config/config.h"
#include "../src/game/entities/car.h"
#include "../src/game/entities/road.h"
#include "../src/game/game.h"
#include "suites.h"

/* The two road lengths differ, so swapping them in the game would be caught. */
#define ROAD_LENGTH_H 10
#define ROAD_LENGTH_V 12

static char path[64];
static config_t* config;
static game_t* game;

/** Builds `game` from a human-mode config with the given horizontal spawn probability. */
static void create_game(const char* spawn_probability_h) {
    FILE* f = fopen(path, "w");
    ck_assert_ptr_nonnull(f);
    fprintf(f,
        "mode=human\n"
        "car_spawn_probability_h=%s\n"
        "car_spawn_probability_v=0\n"
        "road_length_h=%d\n"
        "road_length_v=%d\n"
        "view_width=21\n"
        "view_height=21\n",
        spawn_probability_h, ROAD_LENGTH_H, ROAD_LENGTH_V);
    fclose(f);

    ck_assert_int_eq(config_create(path, &config), CONFIG_STATUS_OK);
    ck_assert_int_eq(game_create(config, &game), GAME_STATUS_OK);
}

static void setup(void) {
    snprintf(path, sizeof(path), "/tmp/semaphore_game_XXXXXX");
    int fd = mkstemp(path);
    ck_assert_int_ge(fd, 0);
    close(fd);

    config = NULL;
    game = NULL;
}

static void teardown(void) {
    game_destroy(&game);
    config_destroy(&config);
    unlink(path);
}

static car_t* car_at(const road_t* road, int index) {
    car_t* car = NULL;
    ck_assert_int_eq(road_get(road, index, &car), ROAD_STATUS_OK);
    return car;
}

START_TEST(test_game_starts_running_with_empty_roads) {
    create_game("0");

    ck_assert_int_eq(game_get_state(game), GAME_STATE_RUNNING);
    ck_assert_int_eq(road_size(game_get_horizontal_road(game)), 0);
    ck_assert_int_eq(road_size(game_get_vertical_road(game)), 0);
}
END_TEST

START_TEST(test_game_no_cars_spawn_with_zero_probability) {
    create_game("0");

    for (int i = 0; i < 5; i++) {
        game_exec(game, GAME_COMMAND_NOOP);
    }

    ck_assert_int_eq(road_size(game_get_horizontal_road(game)), 0);
}
END_TEST

START_TEST(test_game_horizontal_cars_spawn_at_the_vertical_road_length) {
    create_game("2"); /* above 1: spawns on every tick */

    game_exec(game, GAME_COMMAND_NOOP);

    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(road_size(road), 1);
    ck_assert_int_eq(car_get_x(car_at(road, 0)), 0);
    ck_assert_int_eq(car_get_y(car_at(road, 0)), ROAD_LENGTH_V);
}
END_TEST

START_TEST(test_game_spawn_row_does_not_depend_on_the_number_of_cars) {
    create_game("2");

    for (int i = 0; i < 4; i++) {
        game_exec(game, GAME_COMMAND_NOOP);
    }

    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(road_size(road), 4);
    for (int i = 0; i < road_size(road); i++) {
        ck_assert_int_eq(car_get_y(car_at(road, i)), ROAD_LENGTH_V);
    }
}
END_TEST

START_TEST(test_game_horizontal_cars_move_right_every_tick) {
    create_game("2");

    for (int i = 0; i < 3; i++) {
        game_exec(game, GAME_COMMAND_NOOP);
    }

    /* The oldest car has moved 2 times, the newest has just spawned. */
    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(car_get_x(car_at(road, 0)), 2);
    ck_assert_int_eq(car_get_x(car_at(road, 1)), 1);
    ck_assert_int_eq(car_get_x(car_at(road, 2)), 0);
}
END_TEST

Suite* game_suite(void) {
    Suite* s = suite_create("game");
    TCase* tc = tcase_create("core");

    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_game_starts_running_with_empty_roads);
    tcase_add_test(tc, test_game_no_cars_spawn_with_zero_probability);
    tcase_add_test(tc, test_game_horizontal_cars_spawn_at_the_vertical_road_length);
    tcase_add_test(tc, test_game_spawn_row_does_not_depend_on_the_number_of_cars);
    tcase_add_test(tc, test_game_horizontal_cars_move_right_every_tick);

    suite_add_tcase(s, tc);
    return s;
}
