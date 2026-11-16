#include "../src/agents/linear_combination.h"
#include "ai_fixture.h"
#include "suites.h"

/* The fixture roads are 10 (horizontal) and 12 (vertical) cells long. */
#define LINES \
    "agent=linear_combination\nbinarization_method=threshold\nthreshold=0.5\n" \
    "coeficients_h=0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5\n" \
    "coeficients_v=0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5\n"

static void setup(void) {
    fixture_setup(LINES);
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
}

/** Sets the lights with a tick that does nothing else. */
static void set_lights(traffic_light_color_t h, traffic_light_color_t v) {
    game_exec(game, (game_command_t) { .horizontal_light = h, .vertical_light = v });
}

START_TEST(test_linear_combination_empty_roads_keep_the_lights) {
    /* Both scores are 0, so the difference is inside the margin: the lights stay as they are. */
    set_lights(TRAFFIC_LIGHT_GREEN, TRAFFIC_LIGHT_RED);
    game_command_t command = ai_decide(ai, game);
    ck_assert_int_eq(command.horizontal_light, TRAFFIC_LIGHT_GREEN);
    ck_assert_int_eq(command.vertical_light, TRAFFIC_LIGHT_RED);

    set_lights(TRAFFIC_LIGHT_RED, TRAFFIC_LIGHT_GREEN);
    command = ai_decide(ai, game);
    ck_assert_int_eq(command.horizontal_light, TRAFFIC_LIGHT_RED);
    ck_assert_int_eq(command.vertical_light, TRAFFIC_LIGHT_GREEN);
}
END_TEST

START_TEST(test_linear_combination_fresh_game_gets_one_green_light_horizontal) {
    /* No tick has run: both lights are green and both roads are empty, so the scores tie. */
    ck_assert(h_green() && v_green());

    game_command_t command = ai_decide(ai, game);
    ck_assert_int_eq(command.horizontal_light, TRAFFIC_LIGHT_GREEN);
    ck_assert_int_eq(command.vertical_light, TRAFFIC_LIGHT_RED);
}
END_TEST

START_TEST(test_linear_combination_exactly_one_light_is_green_after_the_first_tick) {
    /* The game starts with both lights green, so only the ticks after the first are checked. */
    game_exec(game, ai_decide(ai, game));

    for (int tick = 0; tick < 300; tick++) {
        game_exec(game, ai_decide(ai, game));
        ck_assert_msg(h_green() != v_green(), "tick %d: horizontal %d, vertical %d", tick, h_green(), v_green());
    }
}
END_TEST

START_TEST(test_linear_combination_gives_the_green_to_the_busier_road) {
    /* Cars queue at the red light, so the green keeps moving to the road that has the longer queue. */
    bool h_was_green = false;
    bool v_was_green = false;

    for (int tick = 0; tick < 300; tick++) {
        game_exec(game, ai_decide(ai, game));
        h_was_green = h_was_green || h_green();
        v_was_green = v_was_green || v_green();
    }

    ck_assert(h_was_green);
    ck_assert(v_was_green);
}
END_TEST

START_TEST(test_linear_combination_works_after_config_is_destroyed) {
    /* The coefficients must have been copied: the config frees its own arrays. */
    config_destroy(&config);

    for (int tick = 0; tick < 50; tick++) {
        game_exec(game, ai_decide(ai, game));
    }
}
END_TEST

START_TEST(test_linear_combination_decide_does_not_change_the_game) {
    for (int tick = 0; tick < 20; tick++) {
        game_exec(game, ai_decide(ai, game));
    }

    bool h = h_green();
    bool v = v_green();
    unsigned int crashes = game_get_crashes(game);
    ai_decide(ai, game);

    ck_assert(h == h_green() && v == v_green());
    ck_assert_uint_eq(game_get_crashes(game), crashes);
}
END_TEST

START_TEST(test_linear_combination_runs_with_cars_queued_on_the_stop_cells) {
    /* Spawn probability 0.5: cars queue on the stop cells, at distance 0 from their light. */
    for (int tick = 0; tick < 200; tick++) {
        game_command_t command = ai_decide(ai, game);
        ck_assert(command.horizontal_light == TRAFFIC_LIGHT_GREEN || command.horizontal_light == TRAFFIC_LIGHT_RED);
        ck_assert(command.vertical_light == TRAFFIC_LIGHT_GREEN || command.vertical_light == TRAFFIC_LIGHT_RED);
        game_exec(game, command);
    }
}
END_TEST

/** The numbers of LINES, as arrays, so the tests can build the same agent without a config. */
static void same_numbers_as_lines(double* h, double* v) {
    for (int i = 0; i < 10; i++) {
        h[i] = 0.5;
    }
    for (int i = 0; i < 12; i++) {
        v[i] = 0.5;
    }
}

START_TEST(test_linear_combination_create_from_decides_like_create) {
    double h[10], v[12];
    same_numbers_as_lines(h, v);
    ai_t* from_numbers = NULL;
    ck_assert_int_eq(linear_combination_create_from(0.5, h, 10, v, 12, &from_numbers), AI_STATUS_OK);

    for (int tick = 0; tick < 400; tick++) {
        game_command_t expected = ai_decide(ai, game);
        game_command_t actual = ai_decide(from_numbers, game);
        ck_assert_int_eq(actual.horizontal_light, expected.horizontal_light);
        ck_assert_int_eq(actual.vertical_light, expected.vertical_light);
        game_exec(game, expected);
    }

    ai_destroy(&from_numbers);
}
END_TEST

START_TEST(test_linear_combination_create_from_copies_the_arrays) {
    double h[10], v[12];
    same_numbers_as_lines(h, v);
    ai_t* from_numbers = NULL;
    ck_assert_int_eq(linear_combination_create_from(0.5, h, 10, v, 12, &from_numbers), AI_STATUS_OK);

    /* Wipe the caller's arrays: the agent must keep its own numbers. */
    for (int i = 0; i < 10; i++) {
        h[i] = 1000;
    }
    for (int i = 0; i < 12; i++) {
        v[i] = -1000;
    }

    for (int tick = 0; tick < 300; tick++) {
        game_command_t expected = ai_decide(ai, game);
        game_command_t actual = ai_decide(from_numbers, game);
        ck_assert_int_eq(actual.horizontal_light, expected.horizontal_light);
        ck_assert_int_eq(actual.vertical_light, expected.vertical_light);
        game_exec(game, expected);
    }

    ai_destroy(&from_numbers);
}
END_TEST

START_TEST(test_linear_combination_create_from_accepts_lists_shorter_than_the_road) {
    /* Only the first 2 cars of each road count: the others must not read past the arrays. */
    double h[2] = { 1, 1 };
    double v[1] = { 1 };
    ai_t* short_ai = NULL;
    ck_assert_int_eq(linear_combination_create_from(0.1, h, 2, v, 1, &short_ai), AI_STATUS_OK);

    for (int tick = 0; tick < 300; tick++) {
        game_exec(game, ai_decide(short_ai, game));
    }

    ai_destroy(&short_ai);
}
END_TEST

START_TEST(test_linear_combination_create_from_rejects_missing_arrays_and_zero_lengths) {
    double h[10], v[12];
    same_numbers_as_lines(h, v);
    ai_t* untouched = (ai_t*)h; /* any address: it must come back unchanged */
    ai_t* result = untouched;

    ck_assert_int_eq(linear_combination_create_from(0.5, NULL, 10, v, 12, &result), AI_STATUS_EAGENT);
    ck_assert_int_eq(linear_combination_create_from(0.5, h, 10, NULL, 12, &result), AI_STATUS_EAGENT);
    ck_assert_int_eq(linear_combination_create_from(0.5, h, 0, v, 12, &result), AI_STATUS_EAGENT);
    ck_assert_int_eq(linear_combination_create_from(0.5, h, 10, v, 0, &result), AI_STATUS_EAGENT);
    ck_assert_ptr_eq(result, untouched);
}
END_TEST

Suite* linear_combination_suite(void) {
    Suite* s = suite_create("linear_combination");
    TCase* tc = tcase_create("core");
    tcase_add_checked_fixture(tc, setup, fixture_teardown);

    tcase_add_test(tc, test_linear_combination_empty_roads_keep_the_lights);
    tcase_add_test(tc, test_linear_combination_fresh_game_gets_one_green_light_horizontal);
    tcase_add_test(tc, test_linear_combination_exactly_one_light_is_green_after_the_first_tick);
    tcase_add_test(tc, test_linear_combination_gives_the_green_to_the_busier_road);
    tcase_add_test(tc, test_linear_combination_works_after_config_is_destroyed);
    tcase_add_test(tc, test_linear_combination_decide_does_not_change_the_game);
    tcase_add_test(tc, test_linear_combination_runs_with_cars_queued_on_the_stop_cells);
    tcase_add_test(tc, test_linear_combination_create_from_decides_like_create);
    tcase_add_test(tc, test_linear_combination_create_from_copies_the_arrays);
    tcase_add_test(tc, test_linear_combination_create_from_accepts_lists_shorter_than_the_road);
    tcase_add_test(tc, test_linear_combination_create_from_rejects_missing_arrays_and_zero_lengths);

    suite_add_tcase(s, tc);
    return s;
}
