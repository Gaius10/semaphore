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

/** 'H': only horizontal green, 'V': only vertical green, '-': both red, '+': both green. */
static char lights_symbol(void) {
    if (h_green() && v_green()) return '+';
    if (h_green()) return 'H';
    if (v_green()) return 'V';
    return '-';
}

START_TEST(test_fixed_toggle_swaps_the_lights_every_interval) {
    use_interval(3);

    /*
     * The lights after each tick. H stays green for 3 ticks, then the lights swap on the next
     * tick (H closes and V opens at once), and back again.
     */
    const char* expected = "HHHV" "VVVH" "HHHV";

    for (size_t tick = 0; expected[tick] != '\0'; tick++) {
        game_exec(game, ai_decide(ai, game));
        ck_assert_msg(lights_symbol() == expected[tick], "tick %zu: lights are %c, expected %c", tick, lights_symbol(), expected[tick]);
    }
}
END_TEST

START_TEST(test_fixed_toggle_never_has_both_lights_green) {
    use_interval(2);

    bool seen_h_only = false, seen_v_only = false;
    for (int tick = 0; tick < 200; tick++) {
        game_exec(game, ai_decide(ai, game));

        ck_assert(!(h_green() && v_green()));
        seen_h_only |= h_green() && !v_green();
        seen_v_only |= v_green() && !h_green();
    }

    ck_assert(seen_h_only);
    ck_assert(seen_v_only);
}
END_TEST

START_TEST(test_fixed_toggle_interval_of_one) {
    use_interval(1);

    game_exec(game, ai_decide(ai, game));
    ck_assert_int_eq(lights_symbol(), 'H');

    game_exec(game, ai_decide(ai, game));
    ck_assert_int_eq(lights_symbol(), 'V');
}
END_TEST

START_TEST(test_fixed_toggle_recovers_when_both_lights_match) {
    use_interval(5);

    /* Someone else opens the vertical light: both green. */
    game_exec(game, (game_command_t) { .horizontal_light = TRAFFIC_LIGHT_GREEN, .vertical_light = TRAFFIC_LIGHT_GREEN });
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
