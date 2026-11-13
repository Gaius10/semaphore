#include "ai_fixture.h"
#include "suites.h"

/**
 * The AI interface as the mode sees it, for every agent: ai_create picks the agent from the config,
 * and the rest is the same whichever one it is. Behavior specific to an agent is in its own file.
 */

#define NOOP_LINES "agent=noop\n"
#define FIXED_TOGGLE_LINES "agent=fixed_toggle\ntoggle_interval_ticks=3\n"

static void setup_noop(void) {
    fixture_setup(NOOP_LINES);
}

static void setup_fixed_toggle(void) {
    fixture_setup(FIXED_TOGGLE_LINES);
}

START_TEST(test_ai_create_gives_an_ai) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
    ck_assert_ptr_nonnull(ai);
}
END_TEST

START_TEST(test_ai_outlives_config) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
    config_destroy(&config);
    for (int tick = 0; tick < 10; tick++) {
        game_exec(game, ai_decide(ai, game));
    }
}
END_TEST

START_TEST(test_ai_decide_does_not_change_the_game) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);
    unsigned int crashes = game_get_crashes(game);
    bool h = h_green();
    bool v = v_green();

    for (int tick = 0; tick < 10; tick++) {
        ai_decide(ai, game); /* the command is not run */
    }

    ck_assert_uint_eq(game_get_crashes(game), crashes);
    ck_assert(h == h_green() && v == v_green());
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

    /* The same interface tests, once per agent. */
    void (*setups[])(void) = { setup_noop, setup_fixed_toggle };
    const char* names[] = { "noop", "fixed_toggle" };

    for (int i = 0; i < 2; i++) {
        TCase* tc = tcase_create(names[i]);
        tcase_add_checked_fixture(tc, setups[i], fixture_teardown);

        tcase_add_test(tc, test_ai_create_gives_an_ai);
        tcase_add_test(tc, test_ai_outlives_config);
        tcase_add_test(tc, test_ai_decide_does_not_change_the_game);
        tcase_add_test(tc, test_ai_destroy_nulls_pointer);
        tcase_add_test(tc, test_ai_destroy_null_safe);

        suite_add_tcase(s, tc);
    }

    return s;
}
