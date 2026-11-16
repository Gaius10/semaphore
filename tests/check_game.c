#define _POSIX_C_SOURCE 200809L /* mkstemp is hidden under strict -std=c2x */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/config/config.h"
#include "../src/game/entities/car.h"
#include "../src/game/entities/road.h"
#include "../src/game/entities/traffic_light.h"
#include "../src/game/game.h"
#include "suites.h"

/* The two road lengths differ, so swapping them in the game would be caught. */
#define ROAD_LENGTH_H 10
#define ROAD_LENGTH_V 12

static char path[64];
static config_t* config;
static game_t* game;

/**
 * Builds `game` from a human-mode config with the given spawn probabilities.
 * "0" never spawns and "2" (above 1) spawns on every tick.
 */
static void create_game(const char* spawn_probability_h, const char* spawn_probability_v) {
    FILE* f = fopen(path, "w");
    ck_assert_ptr_nonnull(f);
    fprintf(f,
        "mode=human\n"
        "car_spawn_probability_h=%s\n"
        "car_spawn_probability_v=%s\n"
        "road_length_h=%d\n"
        "road_length_v=%d\n"
        "view_width=21\n"
        "view_height=21\n"
        "tick_ms=200\n",
        spawn_probability_h, spawn_probability_v, ROAD_LENGTH_H, ROAD_LENGTH_V);
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

static traffic_light_color_t opposite(traffic_light_color_t color) {
    return color == TRAFFIC_LIGHT_GREEN ? TRAFFIC_LIGHT_RED : TRAFFIC_LIGHT_GREEN;
}

/** The state the lights show now, as a command. */
static game_command_t current_lights(void) {
    return (game_command_t) {
        .horizontal_light = traffic_light_get_color(game_get_horizontal_light(game)),
        .vertical_light = traffic_light_get_color(game_get_vertical_light(game)),
    };
}

/** Runs a tick that leaves the lights as they are. */
static void keep_lights(void) {
    game_exec(game, current_lights());
}

/** Runs a tick that flips the horizontal light and leaves the vertical one as it is. */
static void toggle_horizontal_light(void) {
    game_command_t command = current_lights();
    command.horizontal_light = opposite(command.horizontal_light);
    game_exec(game, command);
}

/** Runs a tick that flips the vertical light and leaves the horizontal one as it is. */
static void toggle_vertical_light(void) {
    game_command_t command = current_lights();
    command.vertical_light = opposite(command.vertical_light);
    game_exec(game, command);
}

static car_t* car_at(const road_t* road, int index) {
    car_t* car = NULL;
    ck_assert_int_eq(road_get(road, index, &car), ROAD_STATUS_OK);
    return car;
}

static void run_ticks(int ticks) {
    for (int i = 0; i < ticks; i++) {
        keep_lights();
    }
}

START_TEST(test_game_starts_running_with_empty_roads) {
    create_game("0", "0");

    ck_assert_int_eq(game_get_state(game), GAME_STATE_RUNNING);
    ck_assert_int_eq(road_size(game_get_horizontal_road(game)), 0);
    ck_assert_int_eq(road_size(game_get_vertical_road(game)), 0);
}
END_TEST

START_TEST(test_game_no_cars_spawn_with_zero_probability) {
    create_game("0", "0");

    for (int i = 0; i < 5; i++) {
        keep_lights();
    }

    ck_assert_int_eq(road_size(game_get_horizontal_road(game)), 0);
}
END_TEST

START_TEST(test_game_horizontal_cars_spawn_at_the_vertical_road_length) {
    create_game("2", "0");

    keep_lights();

    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(road_size(road), 1);
    ck_assert_int_eq(car_get_x(car_at(road, 0)), 0);
    ck_assert_int_eq(car_get_y(car_at(road, 0)), ROAD_LENGTH_V);
}
END_TEST

START_TEST(test_game_spawn_row_does_not_depend_on_the_number_of_cars) {
    create_game("2", "0");

    for (int i = 0; i < 4; i++) {
        keep_lights();
    }

    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(road_size(road), 4);
    for (int i = 0; i < road_size(road); i++) {
        ck_assert_int_eq(car_get_y(car_at(road, i)), ROAD_LENGTH_V);
    }
}
END_TEST

START_TEST(test_game_horizontal_cars_move_right_every_tick) {
    create_game("2", "0");

    for (int i = 0; i < 3; i++) {
        keep_lights();
    }

    /* The oldest car has moved 2 times, the newest has just spawned. */
    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(car_get_x(car_at(road, 0)), 2);
    ck_assert_int_eq(car_get_x(car_at(road, 1)), 1);
    ck_assert_int_eq(car_get_x(car_at(road, 2)), 0);
}
END_TEST

START_TEST(test_game_numbers_cars_in_spawn_order_across_both_roads) {
    create_game("2", "2");

    keep_lights();
    keep_lights();

    /* Each tick spawns the horizontal car first, then the vertical one. */
    const road_t* h = game_get_horizontal_road(game);
    const road_t* v = game_get_vertical_road(game);
    ck_assert_uint_eq(car_get_id(car_at(h, 0)), 1);
    ck_assert_uint_eq(car_get_id(car_at(v, 0)), 2);
    ck_assert_uint_eq(car_get_id(car_at(h, 1)), 3);
    ck_assert_uint_eq(car_get_id(car_at(v, 1)), 4);
}
END_TEST

START_TEST(test_game_lights_start_both_green) {
    create_game("0", "0");

    const traffic_light_t* h = game_get_horizontal_light(game);
    const traffic_light_t* v = game_get_vertical_light(game);

    ck_assert_int_eq(traffic_light_get_color(h), TRAFFIC_LIGHT_GREEN);
    ck_assert_int_eq(traffic_light_get_color(v), TRAFFIC_LIGHT_GREEN);
}
END_TEST

START_TEST(test_game_stop_cells_are_right_before_the_crossing) {
    create_game("0", "0");

    /* The roads cross at (ROAD_LENGTH_H, ROAD_LENGTH_V). */
    const traffic_light_t* h = game_get_horizontal_light(game);
    const traffic_light_t* v = game_get_vertical_light(game);

    ck_assert_int_eq(traffic_light_get_x(h), ROAD_LENGTH_H - 1);
    ck_assert_int_eq(traffic_light_get_y(h), ROAD_LENGTH_V);
    ck_assert_int_eq(traffic_light_get_x(v), ROAD_LENGTH_H);
    ck_assert_int_eq(traffic_light_get_y(v), ROAD_LENGTH_V - 1);
}
END_TEST

START_TEST(test_game_command_changes_only_the_lights_it_changes) {
    create_game("0", "0");

    /* Both lights start green. */
    toggle_horizontal_light();
    ck_assert_int_eq(traffic_light_get_color(game_get_horizontal_light(game)), TRAFFIC_LIGHT_RED);
    ck_assert_int_eq(traffic_light_get_color(game_get_vertical_light(game)), TRAFFIC_LIGHT_GREEN);

    toggle_vertical_light();
    ck_assert_int_eq(traffic_light_get_color(game_get_horizontal_light(game)), TRAFFIC_LIGHT_RED);
    ck_assert_int_eq(traffic_light_get_color(game_get_vertical_light(game)), TRAFFIC_LIGHT_RED);

    toggle_horizontal_light();
    ck_assert_int_eq(traffic_light_get_color(game_get_horizontal_light(game)), TRAFFIC_LIGHT_GREEN);
    ck_assert_int_eq(traffic_light_get_color(game_get_vertical_light(game)), TRAFFIC_LIGHT_RED);

    keep_lights();
    ck_assert_int_eq(traffic_light_get_color(game_get_horizontal_light(game)), TRAFFIC_LIGHT_GREEN);
    ck_assert_int_eq(traffic_light_get_color(game_get_vertical_light(game)), TRAFFIC_LIGHT_RED);
}
END_TEST

START_TEST(test_game_command_sets_the_state_of_both_lights) {
    create_game("0", "0");

    const game_command_t all_states[] = {
        { TRAFFIC_LIGHT_GREEN, TRAFFIC_LIGHT_GREEN },
        { TRAFFIC_LIGHT_GREEN, TRAFFIC_LIGHT_RED },
        { TRAFFIC_LIGHT_RED, TRAFFIC_LIGHT_GREEN },
        { TRAFFIC_LIGHT_RED, TRAFFIC_LIGHT_RED },
    };

    /* Each command gives the same state whatever state the game was in, and twice in a row too. */
    for (size_t from = 0; from < 4; from++) {
        for (size_t to = 0; to < 4; to++) {
            game_exec(game, all_states[from]);
            game_exec(game, all_states[to]);
            game_exec(game, all_states[to]);

            game_command_t current = current_lights();
            ck_assert_int_eq(current.horizontal_light, all_states[to].horizontal_light);
            ck_assert_int_eq(current.vertical_light, all_states[to].vertical_light);
        }
    }
}
END_TEST

START_TEST(test_game_red_light_queues_cars_back_to_the_spawn) {
    create_game("2", "0");

    toggle_horizontal_light();
    run_ticks(2 * ROAD_LENGTH_H);

    /* One car per cell from the stop cell back to the spawn cell, and no more spawn. */
    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(road_size(road), ROAD_LENGTH_H);
    for (int i = 0; i < road_size(road); i++) {
        ck_assert_int_eq(car_get_x(car_at(road, i)), ROAD_LENGTH_H - 1 - i);
        ck_assert_int_eq(car_get_y(car_at(road, i)), ROAD_LENGTH_V);
    }
}
END_TEST

START_TEST(test_game_vertical_road_starts_queued_at_its_red_light) {
    create_game("0", "2");

    toggle_vertical_light(); /* closes it: both lights start green */
    run_ticks(2 * ROAD_LENGTH_V);

    const road_t* road = game_get_vertical_road(game);
    ck_assert_int_eq(road_size(road), ROAD_LENGTH_V);
    for (int i = 0; i < road_size(road); i++) {
        ck_assert_int_eq(car_get_x(car_at(road, i)), ROAD_LENGTH_H);
        ck_assert_int_eq(car_get_y(car_at(road, i)), ROAD_LENGTH_V - 1 - i);
    }
}
END_TEST

START_TEST(test_game_green_light_releases_the_queue) {
    create_game("2", "0");

    toggle_horizontal_light();
    run_ticks(2 * ROAD_LENGTH_H);
    toggle_horizontal_light();

    /* The whole queue moved on together, which frees the spawn cell for a new car. */
    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(car_get_x(car_at(road, 0)), ROAD_LENGTH_H);
    ck_assert_int_eq(car_get_x(car_at(road, ROAD_LENGTH_H - 1)), 1);
    ck_assert_int_eq(road_size(road), ROAD_LENGTH_H + 1);
}
END_TEST

START_TEST(test_game_red_light_only_holds_the_car_on_the_stop_cell) {
    create_game("2", "0");

    /* The first car is now in the crossing and the second one on the stop cell. */
    run_ticks(ROAD_LENGTH_H + 1);
    toggle_horizontal_light();

    const road_t* road = game_get_horizontal_road(game);
    ck_assert_int_eq(car_get_x(car_at(road, 0)), ROAD_LENGTH_H + 1);
    ck_assert_int_eq(car_get_x(car_at(road, 1)), ROAD_LENGTH_H - 1);
}
END_TEST

START_TEST(test_game_no_crashes_while_one_light_is_closed) {
    create_game("2", "2");

    toggle_vertical_light(); /* closes it: both lights start green */
    run_ticks(3 * ROAD_LENGTH_V);

    ck_assert_uint_eq(game_get_crashes(game), 0);
}
END_TEST

START_TEST(test_game_both_lights_green_counts_crashes_and_keeps_running) {
    create_game("2", "2");

    /*
     * Both roads spawn on every tick. The car spawned on tick t reaches the crossing on tick
     * t + ROAD_LENGTH_H (horizontal) or t + ROAD_LENGTH_V (vertical), so from tick
     * ROAD_LENGTH_V + 1 on both roads have a car in the crossing on every tick.
     */
    run_ticks(ROAD_LENGTH_V + 3);

    ck_assert_uint_eq(game_get_crashes(game), 3);
    ck_assert_int_eq(game_get_state(game), GAME_STATE_RUNNING);
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
    tcase_add_test(tc, test_game_numbers_cars_in_spawn_order_across_both_roads);
    tcase_add_test(tc, test_game_lights_start_both_green);
    tcase_add_test(tc, test_game_stop_cells_are_right_before_the_crossing);
    tcase_add_test(tc, test_game_command_changes_only_the_lights_it_changes);
    tcase_add_test(tc, test_game_command_sets_the_state_of_both_lights);
    tcase_add_test(tc, test_game_red_light_queues_cars_back_to_the_spawn);
    tcase_add_test(tc, test_game_vertical_road_starts_queued_at_its_red_light);
    tcase_add_test(tc, test_game_green_light_releases_the_queue);
    tcase_add_test(tc, test_game_red_light_only_holds_the_car_on_the_stop_cell);
    tcase_add_test(tc, test_game_no_crashes_while_one_light_is_closed);
    tcase_add_test(tc, test_game_both_lights_green_counts_crashes_and_keeps_running);

    suite_add_tcase(s, tc);
    return s;
}
