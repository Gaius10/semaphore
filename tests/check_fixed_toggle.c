#include "ai_fixture.h"
#include "suites.h"

/** Rebuilds the config, game and AI with the fixed toggle agent at the given interval. */
static void use_interval(unsigned int interval) {
    char lines[64];
    snprintf(lines, sizeof(lines), "agent=fixed_toggle\ntoggle_interval_ticks=%u\n", interval);

    fixture_teardown();
    fixture_setup(lines);
    start_h_green_v_red();
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
}

static void setup(void) {
    fixture_setup("agent=noop\n"); /* a valid starting point: each test picks its interval */
}

START_TEST(test_fixed_toggle_swaps_the_lights_every_interval) {
    use_interval(3);

    const game_command_t expected[] = {
        /* H green for 3 ticks, then H closes and V opens on the next tick */
        GAME_COMMAND_NOOP, GAME_COMMAND_NOOP, GAME_COMMAND_NOOP,
        GAME_COMMAND_HORIZONTAL_TOGGLE, GAME_COMMAND_VERTICAL_TOGGLE,
        /* V green for 3 ticks, then V closes and H opens */
        GAME_COMMAND_NOOP, GAME_COMMAND_NOOP, GAME_COMMAND_NOOP,
        GAME_COMMAND_VERTICAL_TOGGLE, GAME_COMMAND_HORIZONTAL_TOGGLE,
        /* and back again */
        GAME_COMMAND_NOOP, GAME_COMMAND_NOOP, GAME_COMMAND_NOOP,
        GAME_COMMAND_HORIZONTAL_TOGGLE, GAME_COMMAND_VERTICAL_TOGGLE,
    };

    for (size_t tick = 0; tick < sizeof(expected) / sizeof(expected[0]); tick++) {
        game_command_t command = ai_decide(ai, game);
        ck_assert_msg(command == expected[tick], "tick %zu: got command %d, expected %d", tick, command, expected[tick]);
        game_exec(game, command);
    }
}
END_TEST

START_TEST(test_fixed_toggle_never_has_both_lights_green) {
    use_interval(2);

    bool seen_h_only = false, seen_v_only = false, seen_all_red = false;
    for (int tick = 0; tick < 200; tick++) {
        game_exec(game, ai_decide(ai, game));

        ck_assert(!(h_green() && v_green()));
        seen_h_only |= h_green() && !v_green();
        seen_v_only |= v_green() && !h_green();
        seen_all_red |= !h_green() && !v_green();
    }

    ck_assert(seen_h_only);
    ck_assert(seen_v_only);
    ck_assert(seen_all_red);
}
END_TEST

START_TEST(test_fixed_toggle_interval_of_one) {
    use_interval(1);

    ck_assert_int_eq(ai_decide(ai, game), GAME_COMMAND_NOOP);
    ck_assert_int_eq(ai_decide(ai, game), GAME_COMMAND_HORIZONTAL_TOGGLE);
}
END_TEST

START_TEST(test_fixed_toggle_recovers_when_both_lights_match) {
    use_interval(5);

    /* Someone else opens the vertical light: both green. */
    game_exec(game, GAME_COMMAND_VERTICAL_TOGGLE);
    ck_assert(h_green() && v_green());

    /* The agent closes one of them on its first tick, without waiting for the interval. */
    game_exec(game, ai_decide(ai, game));
    ck_assert(h_green() != v_green());
}
END_TEST

Suite* fixed_toggle_suite(void) {
    Suite* s = suite_create("fixed_toggle");
    TCase* tc = tcase_create("core");
    tcase_add_checked_fixture(tc, setup, fixture_teardown);

    tcase_add_test(tc, test_fixed_toggle_swaps_the_lights_every_interval);
    tcase_add_test(tc, test_fixed_toggle_never_has_both_lights_green);
    tcase_add_test(tc, test_fixed_toggle_interval_of_one);
    tcase_add_test(tc, test_fixed_toggle_recovers_when_both_lights_match);

    suite_add_tcase(s, tc);
    return s;
}
