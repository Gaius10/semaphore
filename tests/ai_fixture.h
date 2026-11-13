#ifndef AI_FIXTURE_H
#define AI_FIXTURE_H

#define _POSIX_C_SOURCE 200809L /* mkstemp is hidden under strict -std=c2x */

#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/ai/ai.h"
#include "../src/config/config.h"
#include "../src/game/game.h"

/**
 * Shared by the AI test files (check_ai.c for the dispatch, one check_<agent>.c per agent): a
 * complete AI-mode config for a given agent, and the game and AI built from it. Header-only and
 * static, so each test file gets its own copy of the state.
 */
static char fixture_path[64];
static config_t* config;
static game_t* game;
static ai_t* ai;

/** Writes an AI-mode config with the given agent lines to a temp file and builds `config` and `game` from it. */
static void fixture_setup(const char* agent_lines) {
    snprintf(fixture_path, sizeof(fixture_path), "/tmp/semaphore_ai_XXXXXX");
    int fd = mkstemp(fixture_path);
    ck_assert_int_ge(fd, 0);
    close(fd);

    FILE* f = fopen(fixture_path, "w");
    ck_assert_ptr_nonnull(f);
    fputs("mode=ai\n", f);
    fputs(agent_lines, f);
    fputs(
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
    ck_assert_int_eq(config_create(fixture_path, &config), CONFIG_STATUS_OK);
    ck_assert_int_eq(game_create(config, &game), GAME_STATUS_OK);
}

static void fixture_teardown(void) {
    ai_destroy(&ai);
    game_destroy(&game);
    config_destroy(&config);
    unlink(fixture_path);
}

static bool h_green(void) {
    return traffic_light_get_color(game_get_horizontal_light(game)) == TRAFFIC_LIGHT_GREEN;
}

static bool v_green(void) {
    return traffic_light_get_color(game_get_vertical_light(game)) == TRAFFIC_LIGHT_GREEN;
}

/**
 * Puts the lights in the starting state the game documents (horizontal green, vertical red), so
 * the tests do not depend on how the game happens to set them up.
 */
static void start_h_green_v_red(void) {
    if (!h_green()) {
        game_exec(game, GAME_COMMAND_HORIZONTAL_TOGGLE);
    }
    if (v_green()) {
        game_exec(game, GAME_COMMAND_VERTICAL_TOGGLE);
    }
}

#endif
