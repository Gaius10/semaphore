#define _POSIX_C_SOURCE 200809L /* mkstemp is hidden under strict -std=c2x */

#include <check.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/**
 * A complete human-mode config, in pieces so a test can leave one out or change it.
 * The values differ from each other so that a swapped pair of getters would be caught.
 */
#define MODE_HUMAN "mode=human\n"
#define MODE_AI "mode=ai\n"
#define SPAWN_H "car_spawn_probability_h=0.25\n"
#define SPAWN_V "car_spawn_probability_v=0.75\n"
#define ROAD_H "road_length_h=10\n"
#define ROAD_V "road_length_v=12\n"
#define VIEW_W "view_width=21\n"
#define VIEW_H "view_height=15\n"
#define TICK "tick_ms=200\n"
#define AGENT_NOOP "agent=noop\n"
#define AGENT_FIXED "agent=fixed_toggle\n"
#define INTERVAL "toggle_interval_ticks=5\n"
#define BINARIZATION "binarization_method=threshold\n"
#define THRESHOLD "threshold=0.5\n"
#define AGENT_LINEAR "agent=linear_combination\n"
/* One coefficient per road cell: ROAD_H is 10 and ROAD_V is 12. The values differ so swapped lists are caught. */
#define COEF_H "coeficients_h=1, 2.5,3 ,4,5,6,7,8,9,10\n"
#define COEF_V "coeficients_v=11,12,13,14,15,16,17,18,19,20,21,22\n"
#define NO_MODE SPAWN_H SPAWN_V ROAD_H ROAD_V VIEW_W VIEW_H TICK
#define FULL_HUMAN MODE_HUMAN NO_MODE

/** The training keys, one per line, so a test can leave one out. The values differ so swapped getters are caught. */
#define MODE_TRAINING_LINE "mode=training\n"
static const char* const TRAINING_KEYS[] = {
    "population_size=30\n",
    "generations=40\n",
    "episodes_per_candidate=3\n",
    "episode_ticks=500\n",
    "crash_penalty=5.5\n",
    "elite_count=2\n",
    "tournament_size=4\n",
    "mutation_rate=0.1\n",
    "mutation_sigma=0.2\n",
    "coeficient_max=2.5\n",
    "threshold_max=4\n",
    "seed=1234\n",
    "workers=6\n",
    "training_output_dir=data\n",
};
#define TRAINING_KEY_COUNT (sizeof(TRAINING_KEYS) / sizeof(TRAINING_KEYS[0]))
/** Training plays the world only: no view and no tick_ms. */
#define TRAINING_WORLD SPAWN_H SPAWN_V ROAD_H ROAD_V

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
        NO_MODE), CONFIG_STATUS_OK);

    ck_assert_int_eq(config_get_mode(config), MODE_HUMAN_PLAYER);
    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_v, 0.75, 1e-9);
    ck_assert_uint_eq(config_get_human_player(config).road_length_h, 10);
    ck_assert_uint_eq(config_get_human_player(config).road_length_v, 12);
    ck_assert_int_eq(config_get_view(config).width, 21);
    ck_assert_int_eq(config_get_view(config).height, 15);
    ck_assert_uint_eq(config_get_tick_ms(config), 200);
}
END_TEST

START_TEST(test_config_parses_ai_mode) {
    ck_assert_int_eq(create_from(MODE_AI AGENT_NOOP NO_MODE), CONFIG_STATUS_OK);
    ck_assert_int_eq(config_get_mode(config), MODE_AI_PLAYER);
    ck_assert_int_eq(config_get_ai_player(config).agent, CONFIG_AGENT_NOOP);
    /* The AI plays the same world as the human, so it reads the same keys. */
    ck_assert_uint_eq(config_get_human_player(config).road_length_h, 10);
    ck_assert_uint_eq(config_get_human_player(config).road_length_v, 12);
    ck_assert_int_eq(config_get_view(config).width, 21);
    ck_assert_uint_eq(config_get_tick_ms(config), 200);
}
END_TEST

START_TEST(test_config_ai_mode_needs_the_world_keys) {
    ck_assert_int_eq(create_from("mode=ai\n"), CONFIG_STATUS_EMISSING);
}
END_TEST

START_TEST(test_config_ai_mode_missing_tick_ms) {
    ck_assert_int_eq(create_from(MODE_AI AGENT_NOOP SPAWN_H SPAWN_V ROAD_H ROAD_V VIEW_W VIEW_H), CONFIG_STATUS_EMISSING);
}
END_TEST

START_TEST(test_config_ai_mode_needs_an_agent) {
    ck_assert_int_eq(create_from(MODE_AI NO_MODE), CONFIG_STATUS_EMISSING);
}
END_TEST

START_TEST(test_config_invalid_agent) {
    ck_assert_int_eq(create_from(MODE_AI "agent=genius\n" NO_MODE), CONFIG_STATUS_EPARSE);
}
END_TEST

START_TEST(test_config_parses_fixed_toggle_agent) {
    ck_assert_int_eq(create_from(MODE_AI AGENT_FIXED INTERVAL NO_MODE), CONFIG_STATUS_OK);
    ck_assert_int_eq(config_get_ai_player(config).agent, CONFIG_AGENT_FIXED_TOGGLE);
    ck_assert_uint_eq(config_get_ai_player(config).toggle_interval_ticks, 5);
}
END_TEST

START_TEST(test_config_fixed_toggle_needs_an_interval) {
    ck_assert_int_eq(create_from(MODE_AI AGENT_FIXED NO_MODE), CONFIG_STATUS_EMISSING);
}
END_TEST

START_TEST(test_config_invalid_toggle_interval) {
    ck_assert_int_eq(create_from(MODE_AI AGENT_FIXED "toggle_interval_ticks=0\n" NO_MODE), CONFIG_STATUS_EPARSE);
    ck_assert_int_eq(create_from(MODE_AI AGENT_FIXED "toggle_interval_ticks=-2\n" NO_MODE), CONFIG_STATUS_EPARSE);
}
END_TEST

START_TEST(test_config_noop_agent_does_not_need_an_interval) {
    ck_assert_int_eq(create_from(MODE_AI AGENT_NOOP NO_MODE), CONFIG_STATUS_OK);
}
END_TEST

START_TEST(test_config_parses_linear_combination_agent) {
    ck_assert_int_eq(create_from(MODE_AI AGENT_LINEAR BINARIZATION THRESHOLD COEF_H COEF_V NO_MODE), CONFIG_STATUS_OK);

    config_ai_player_t ai = config_get_ai_player(config);
    ck_assert_int_eq(ai.agent, CONFIG_AGENT_LINEAR_COMBINATION);
    ck_assert_int_eq(ai.binarization_method, BINARIZATION_METHOD_THRESHOLD);
    ck_assert_double_eq_tol(ai.threshold, 0.5, 1e-9);

    const double* h = ai.coeficients_h;
    const double* v = ai.coeficients_v;
    ck_assert_ptr_nonnull(h);
    ck_assert_ptr_nonnull(v);

    /* Spaces around the numbers are fine, and the lists are not swapped. */
    ck_assert_double_eq_tol(h[0], 1.0, 1e-9);
    ck_assert_double_eq_tol(h[1], 2.5, 1e-9);
    ck_assert_double_eq_tol(h[2], 3.0, 1e-9);
    ck_assert_double_eq_tol(h[9], 10.0, 1e-9);
    ck_assert_double_eq_tol(v[0], 11.0, 1e-9);
    ck_assert_double_eq_tol(v[11], 22.0, 1e-9);
}
END_TEST

START_TEST(test_config_linear_combination_needs_its_keys) {
    const char* const without[] = {
        /* coeficients_h */ MODE_AI AGENT_LINEAR BINARIZATION THRESHOLD COEF_V NO_MODE,
        /* coeficients_v */ MODE_AI AGENT_LINEAR BINARIZATION THRESHOLD COEF_H NO_MODE,
        /* threshold */ MODE_AI AGENT_LINEAR BINARIZATION COEF_H COEF_V NO_MODE,
    };

    for (size_t i = 0; i < sizeof(without) / sizeof(without[0]); i++) {
        ck_assert_msg(create_from(without[i]) == CONFIG_STATUS_EMISSING, "case %zu", i);
        ck_assert_ptr_null(config);
    }
}
END_TEST

START_TEST(test_config_linear_combination_needs_one_coeficient_per_cell) {
    const char* const wrong[] = {
        MODE_AI AGENT_LINEAR BINARIZATION THRESHOLD "coeficients_h=1,2,3,4,5,6,7,8,9\n" COEF_V NO_MODE,
        MODE_AI AGENT_LINEAR BINARIZATION THRESHOLD "coeficients_h=1,2,3,4,5,6,7,8,9,10,11\n" COEF_V NO_MODE,
        MODE_AI AGENT_LINEAR BINARIZATION THRESHOLD COEF_H "coeficients_v=1,2,3,4,5,6,7,8,9,10,11\n" NO_MODE,
        /* The horizontal list given for the vertical road: the roads have different lengths. */
        MODE_AI AGENT_LINEAR BINARIZATION THRESHOLD COEF_H "coeficients_v=1,2,3,4,5,6,7,8,9,10\n" NO_MODE,
    };

    for (size_t i = 0; i < sizeof(wrong) / sizeof(wrong[0]); i++) {
        ck_assert_msg(create_from(wrong[i]) == CONFIG_STATUS_EPARSE, "case %zu", i);
        ck_assert_ptr_null(config);
    }
}
END_TEST

START_TEST(test_config_invalid_coeficients) {
    const char* const bad[] = {
        "coeficients_h=\n",       /* empty list */
        "coeficients_h= \n",      /* blank list */
        "coeficients_h=1,,3\n",   /* empty entry */
        "coeficients_h=1,2,\n",   /* trailing comma */
        "coeficients_h=,1,2\n",   /* leading comma */
        "coeficients_h=1 2,3\n",  /* missing comma */
        "coeficients_h=a,b\n",    /* not numbers */
        "coeficients_h=1,2x\n",   /* junk after a number */
        "coeficients_h=1,nan\n",  /* not finite */
        "coeficients_h=1,inf\n",
    };

    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        char contents[512];
        snprintf(contents, sizeof(contents), "%s%s", FULL_HUMAN, bad[i]);
        ck_assert_msg(create_from(contents) == CONFIG_STATUS_EPARSE, "case %zu: %s", i, bad[i]);
        ck_assert_ptr_null(config);
    }
}
END_TEST

START_TEST(test_config_coeficients_key_order_does_not_matter) {
    /* The lists come before the road lengths and the agent, so their size is checked after the whole file. */
    ck_assert_int_eq(create_from(COEF_V COEF_H THRESHOLD BINARIZATION AGENT_LINEAR NO_MODE MODE_AI), CONFIG_STATUS_OK);
    ck_assert_double_eq_tol(config_get_ai_player(config).coeficients_h[0], 1.0, 1e-9);
}
END_TEST

START_TEST(test_config_a_repeated_coeficients_key_replaces_the_list) {
    ck_assert_int_eq(create_from(
        MODE_AI AGENT_LINEAR BINARIZATION THRESHOLD
        "coeficients_h=7,7,7\n" COEF_H COEF_V NO_MODE), CONFIG_STATUS_OK);

    ck_assert_double_eq_tol(config_get_ai_player(config).coeficients_h[0], 1.0, 1e-9);
    ck_assert_double_eq_tol(config_get_ai_player(config).coeficients_h[9], 10.0, 1e-9);
}
END_TEST

START_TEST(test_config_other_agents_do_not_need_coeficients) {
    ck_assert_int_eq(create_from(MODE_AI AGENT_FIXED INTERVAL NO_MODE), CONFIG_STATUS_OK);
    ck_assert_ptr_null(config_get_ai_player(config).coeficients_h);
    ck_assert_ptr_null(config_get_ai_player(config).coeficients_v);
    config_destroy(&config);

    /* A list of the wrong size is ignored, since this agent does not use it. */
    ck_assert_int_eq(create_from(MODE_AI AGENT_NOOP "coeficients_h=1,2\n" NO_MODE), CONFIG_STATUS_OK);
}
END_TEST

/**
 * A complete training config, with the key at `skip` left out (pass TRAINING_KEY_COUNT to leave none
 * out) and `extra` added at the end, so a later line can replace a key.
 */
static const char* training_text(size_t skip, const char* extra) {
    static char text[2048];
    text[0] = '\0';
    strcat(text, MODE_TRAINING_LINE TRAINING_WORLD);
    for (size_t i = 0; i < TRAINING_KEY_COUNT; i++) {
        if (i != skip) {
            strcat(text, TRAINING_KEYS[i]);
        }
    }
    strcat(text, extra);
    return text;
}

START_TEST(test_config_parses_training_mode) {
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "")), CONFIG_STATUS_OK);
    ck_assert_int_eq(config_get_mode(config), MODE_TRAINING);

    config_training_t training = config_get_training(config);
    ck_assert_uint_eq(training.population_size, 30);
    ck_assert_uint_eq(training.generations, 40);
    ck_assert_uint_eq(training.episodes_per_candidate, 3);
    ck_assert_uint_eq(training.episode_ticks, 500);
    ck_assert_double_eq(training.crash_penalty, 5.5);
    ck_assert_uint_eq(training.elite_count, 2);
    ck_assert_uint_eq(training.tournament_size, 4);
    ck_assert_double_eq(training.mutation_rate, 0.1);
    ck_assert_double_eq(training.mutation_sigma, 0.2);
    ck_assert_double_eq(training.coeficient_max, 2.5);
    ck_assert_double_eq(training.threshold_max, 4.0);
    ck_assert_uint_eq(training.seed, 1234);
    ck_assert_uint_eq(training.workers, 6);
    ck_assert_str_eq(training.output_dir, "data");

    /* It plays the same world as the other modes. */
    config_human_player_t world = config_get_human_player(config);
    ck_assert_uint_eq(world.road_length_h, 10);
    ck_assert_uint_eq(world.road_length_v, 12);
}
END_TEST

START_TEST(test_config_training_mode_needs_every_training_key) {
    for (size_t skip = 0; skip < TRAINING_KEY_COUNT; skip++) {
        config_destroy(&config);
        ck_assert_msg(create_from(training_text(skip, "")) == CONFIG_STATUS_EMISSING, "left out: %s", TRAINING_KEYS[skip]);
        ck_assert_ptr_null(config);
    }
}
END_TEST

START_TEST(test_config_training_mode_needs_the_world_keys) {
    const char* world[] = { SPAWN_H, SPAWN_V, ROAD_H, ROAD_V };
    for (size_t skip = 0; skip < 4; skip++) {
        char text[2048] = MODE_TRAINING_LINE;
        for (size_t i = 0; i < 4; i++) {
            if (i != skip) strcat(text, world[i]);
        }
        for (size_t i = 0; i < TRAINING_KEY_COUNT; i++) {
            strcat(text, TRAINING_KEYS[i]);
        }

        config_destroy(&config);
        ck_assert_msg(create_from(text) == CONFIG_STATUS_EMISSING, "left out: %s", world[skip]);
    }

    /* Only the mode is not enough either. */
    config_destroy(&config);
    ck_assert_int_eq(create_from(MODE_TRAINING_LINE), CONFIG_STATUS_EMISSING);
}
END_TEST

START_TEST(test_config_training_mode_does_not_need_the_view_or_the_clock) {
    /* training_text has no view_width, view_height or tick_ms. */
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "")), CONFIG_STATUS_OK);
}
END_TEST

START_TEST(test_config_other_modes_do_not_need_the_training_keys) {
    ck_assert_int_eq(create_from(FULL_HUMAN), CONFIG_STATUS_OK);
    config_destroy(&config);
    ck_assert_int_eq(create_from(MODE_AI AGENT_NOOP NO_MODE), CONFIG_STATUS_OK);
}
END_TEST

START_TEST(test_config_training_invalid_values) {
    const char* invalid[] = {
        "population_size=1\n", "population_size=0\n", "population_size=-4\n", "population_size=ten\n",
        "generations=0\n", "generations=2.5\n",
        "episodes_per_candidate=0\n",
        "episode_ticks=0\n",
        "crash_penalty=-0.5\n", "crash_penalty=nan\n", "crash_penalty=inf\n", "crash_penalty=lots\n",
        "elite_count=0\n",
        "tournament_size=0\n",
        "mutation_rate=-0.1\n", "mutation_rate=1.5\n", "mutation_rate=nan\n",
        "mutation_sigma=0\n", "mutation_sigma=-1\n", "mutation_sigma=inf\n",
        "coeficient_max=0\n", "coeficient_max=-2\n", "coeficient_max=inf\n",
        "threshold_max=0\n", "threshold_max=-2\n", "threshold_max=nan\n",
        "seed=0\n", "seed=-1\n",
        "workers=0\n", "workers=many\n",
        "training_output_dir=   \n",
    };

    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        config_destroy(&config);
        ck_assert_msg(create_from(training_text(TRAINING_KEY_COUNT, invalid[i])) == CONFIG_STATUS_EPARSE, "accepted: %s", invalid[i]);
        ck_assert_ptr_null(config);
    }
}
END_TEST

START_TEST(test_config_training_limits_are_inclusive_where_documented) {
    /* population 2, the smallest one that can select; elites and tournament fill it up. */
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "population_size=2\nelite_count=1\ntournament_size=2\n")), CONFIG_STATUS_OK);

    config_destroy(&config);
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "mutation_rate=0\ncrash_penalty=0\n")), CONFIG_STATUS_OK);
    ck_assert_double_eq(config_get_training(config).mutation_rate, 0.0);

    config_destroy(&config);
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "mutation_rate=1\n")), CONFIG_STATUS_OK);
}
END_TEST

START_TEST(test_config_training_elites_and_tournament_must_fit_in_the_population) {
    /* Population 10: all of them as elites leaves no room for children. */
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "population_size=10\nelite_count=10\n")), CONFIG_STATUS_EPARSE);
    config_destroy(&config);
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "population_size=10\nelite_count=9\n")), CONFIG_STATUS_OK);

    config_destroy(&config);
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "population_size=10\ntournament_size=11\n")), CONFIG_STATUS_EPARSE);
    config_destroy(&config);
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "population_size=10\ntournament_size=10\n")), CONFIG_STATUS_OK);

    /* Key order does not matter: the check runs once the whole file is read. */
    config_destroy(&config);
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "elite_count=20\npopulation_size=40\n")), CONFIG_STATUS_OK);
}
END_TEST

START_TEST(test_config_training_output_dir_is_trimmed) {
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "training_output_dir=  out/runs \r\n")), CONFIG_STATUS_OK);
    ck_assert_str_eq(config_get_training(config).output_dir, "out/runs");
}
END_TEST

START_TEST(test_config_training_getter_outlives_config) {
    ck_assert_int_eq(create_from(training_text(TRAINING_KEY_COUNT, "")), CONFIG_STATUS_OK);
    config_training_t training = config_get_training(config);
    config_destroy(&config);

    ck_assert_uint_eq(training.population_size, 30);
    ck_assert_str_eq(training.output_dir, "data");
}
END_TEST

START_TEST(test_config_key_order_does_not_matter) {
    ck_assert_int_eq(create_from(VIEW_H TICK VIEW_W ROAD_V ROAD_H SPAWN_V SPAWN_H MODE_HUMAN), CONFIG_STATUS_OK);

    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_v, 0.75, 1e-9);
}
END_TEST

START_TEST(test_config_skips_comments_and_blank_lines) {
    ck_assert_int_eq(create_from(
        "# hash comment\n"
        "; semicolon comment\n"
        "\n"
        FULL_HUMAN), CONFIG_STATUS_OK);

    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
}
END_TEST

START_TEST(test_config_ignores_unknown_keys) {
    ck_assert_int_eq(create_from(FULL_HUMAN "something_else=1\n"), CONFIG_STATUS_OK);
}
END_TEST

START_TEST(test_config_accepts_trailing_whitespace) {
    ck_assert_int_eq(create_from(MODE_HUMAN "car_spawn_probability_h=0.25 \r\n" SPAWN_V ROAD_H ROAD_V VIEW_W VIEW_H TICK), CONFIG_STATUS_OK);
    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
}
END_TEST

START_TEST(test_config_empty_file_is_missing_mode) {
    ck_assert_int_eq(create_from(""), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_missing_mode) {
    ck_assert_int_eq(create_from(NO_MODE), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_spawn_h) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_V ROAD_H ROAD_V VIEW_W VIEW_H TICK), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_spawn_v) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H ROAD_H ROAD_V VIEW_W VIEW_H TICK), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_road_length_h) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V ROAD_V VIEW_W VIEW_H TICK), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_road_length_v) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V ROAD_H VIEW_W VIEW_H TICK), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_view_width) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V ROAD_H ROAD_V VIEW_H TICK), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_view_height) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V ROAD_H ROAD_V VIEW_W TICK), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_human_mode_missing_tick_ms) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V ROAD_H ROAD_V VIEW_W VIEW_H), CONFIG_STATUS_EMISSING);
    ck_assert_ptr_null(config);
}
END_TEST

START_TEST(test_config_invalid_tick_ms) {
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V ROAD_H ROAD_V VIEW_W VIEW_H "tick_ms=0\n"), CONFIG_STATUS_EPARSE);
    ck_assert_ptr_null(config);
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

START_TEST(test_config_invalid_sizes) {
    /*
     * Lengths and view sizes are whole numbers from 1 to INT_MAX: zero, negatives, fractions, junk
     * and overflow are out. "4294967293" is "-3" wrapped to unsigned, which strtoul would accept.
     */
    const char* bad[] = {"0", "-3", "abc", "1.5", "5x", "", "99999999999999999999", "2147483648", "4294967293"};

    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        char contents[256];

        snprintf(contents, sizeof(contents), MODE_HUMAN SPAWN_H SPAWN_V ROAD_V VIEW_W VIEW_H TICK "road_length_h=%s\n", bad[i]);
        ck_assert_int_eq(create_from(contents), CONFIG_STATUS_EPARSE);
        ck_assert_ptr_null(config);

        snprintf(contents, sizeof(contents), MODE_HUMAN SPAWN_H SPAWN_V ROAD_H ROAD_V VIEW_H TICK "view_width=%s\n", bad[i]);
        ck_assert_int_eq(create_from(contents), CONFIG_STATUS_EPARSE);
        ck_assert_ptr_null(config);
    }
}
END_TEST

START_TEST(test_config_road_length_accepts_int_max) {
    /* The largest length that still fits the game's (int) coordinates. */
    ck_assert_int_eq(create_from(MODE_HUMAN SPAWN_H SPAWN_V ROAD_V VIEW_W VIEW_H TICK "road_length_h=2147483647\n"), CONFIG_STATUS_OK);
    ck_assert_uint_eq(config_get_human_player(config).road_length_h, INT_MAX);
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
    ck_assert_int_eq(create_from(FULL_HUMAN), CONFIG_STATUS_OK);

    config_human_player_t copy = config_get_human_player(config);
    copy.car_spawn_probability_h = 0.99;

    ck_assert_double_eq_tol(config_get_human_player(config).car_spawn_probability_h, 0.25, 1e-9);
}
END_TEST

START_TEST(test_config_getter_outlives_config) {
    ck_assert_int_eq(create_from(FULL_HUMAN), CONFIG_STATUS_OK);

    config_human_player_t copy = config_get_human_player(config);
    config_destroy(&config);

    ck_assert_double_eq_tol(copy.car_spawn_probability_v, 0.75, 1e-9);
}
END_TEST

START_TEST(test_config_destroy_nulls_pointer) {
    ck_assert_int_eq(create_from(FULL_HUMAN), CONFIG_STATUS_OK);

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
}
END_TEST

START_TEST(test_config_record_dir_is_optional) {
    ck_assert_int_eq(create_from(FULL_HUMAN), CONFIG_STATUS_OK);
    ck_assert_str_eq(config_get_record_dir(config), "");
}
END_TEST

START_TEST(test_config_record_dir_is_read_without_surrounding_spaces) {
    ck_assert_int_eq(create_from(FULL_HUMAN "record_dir=  recordings \n"), CONFIG_STATUS_OK);
    ck_assert_str_eq(config_get_record_dir(config), "recordings");
}
END_TEST

START_TEST(test_config_record_dir_cannot_be_empty) {
    ck_assert_int_eq(create_from(FULL_HUMAN "record_dir=   \n"), CONFIG_STATUS_EPARSE);
}
END_TEST

Suite* config_suite(void) {
    Suite* s = suite_create("config");
    TCase* tc = tcase_create("core");

    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_config_parses_human_mode);
    tcase_add_test(tc, test_config_parses_ai_mode);
    tcase_add_test(tc, test_config_ai_mode_needs_the_world_keys);
    tcase_add_test(tc, test_config_ai_mode_missing_tick_ms);
    tcase_add_test(tc, test_config_ai_mode_needs_an_agent);
    tcase_add_test(tc, test_config_invalid_agent);
    tcase_add_test(tc, test_config_parses_fixed_toggle_agent);
    tcase_add_test(tc, test_config_fixed_toggle_needs_an_interval);
    tcase_add_test(tc, test_config_invalid_toggle_interval);
    tcase_add_test(tc, test_config_noop_agent_does_not_need_an_interval);
    tcase_add_test(tc, test_config_parses_linear_combination_agent);
    tcase_add_test(tc, test_config_linear_combination_needs_its_keys);
    tcase_add_test(tc, test_config_linear_combination_needs_one_coeficient_per_cell);
    tcase_add_test(tc, test_config_invalid_coeficients);
    tcase_add_test(tc, test_config_coeficients_key_order_does_not_matter);
    tcase_add_test(tc, test_config_a_repeated_coeficients_key_replaces_the_list);
    tcase_add_test(tc, test_config_other_agents_do_not_need_coeficients);
    tcase_add_test(tc, test_config_parses_training_mode);
    tcase_add_test(tc, test_config_training_mode_needs_every_training_key);
    tcase_add_test(tc, test_config_training_mode_needs_the_world_keys);
    tcase_add_test(tc, test_config_training_mode_does_not_need_the_view_or_the_clock);
    tcase_add_test(tc, test_config_other_modes_do_not_need_the_training_keys);
    tcase_add_test(tc, test_config_training_invalid_values);
    tcase_add_test(tc, test_config_training_limits_are_inclusive_where_documented);
    tcase_add_test(tc, test_config_training_elites_and_tournament_must_fit_in_the_population);
    tcase_add_test(tc, test_config_record_dir_is_optional);
    tcase_add_test(tc, test_config_record_dir_is_read_without_surrounding_spaces);
    tcase_add_test(tc, test_config_record_dir_cannot_be_empty);
    tcase_add_test(tc, test_config_training_output_dir_is_trimmed);
    tcase_add_test(tc, test_config_training_getter_outlives_config);
    tcase_add_test(tc, test_config_key_order_does_not_matter);
    tcase_add_test(tc, test_config_skips_comments_and_blank_lines);
    tcase_add_test(tc, test_config_ignores_unknown_keys);
    tcase_add_test(tc, test_config_accepts_trailing_whitespace);
    tcase_add_test(tc, test_config_empty_file_is_missing_mode);
    tcase_add_test(tc, test_config_missing_mode);
    tcase_add_test(tc, test_config_human_mode_missing_spawn_h);
    tcase_add_test(tc, test_config_human_mode_missing_spawn_v);
    tcase_add_test(tc, test_config_human_mode_missing_road_length_h);
    tcase_add_test(tc, test_config_human_mode_missing_road_length_v);
    tcase_add_test(tc, test_config_human_mode_missing_view_width);
    tcase_add_test(tc, test_config_human_mode_missing_view_height);
    tcase_add_test(tc, test_config_human_mode_missing_tick_ms);
    tcase_add_test(tc, test_config_invalid_tick_ms);
    tcase_add_test(tc, test_config_unknown_mode);
    tcase_add_test(tc, test_config_invalid_probability);
    tcase_add_test(tc, test_config_invalid_sizes);
    tcase_add_test(tc, test_config_road_length_accepts_int_max);
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
