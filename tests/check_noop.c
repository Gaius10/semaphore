#include "ai_fixture.h"
#include "suites.h"

static void setup(void) {
    fixture_setup("agent=noop\n");
}

START_TEST(test_noop_always_keeps_the_lights) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);

    for (int tick = 0; tick < 50; tick++) {
        bool h_before = h_green();
        bool v_before = v_green();
        game_command_t command = ai_decide(ai, game);
        ck_assert((command.horizontal_light == TRAFFIC_LIGHT_GREEN) == h_before);
        ck_assert((command.vertical_light == TRAFFIC_LIGHT_GREEN) == v_before);
        game_exec(game, command);
    }
}
END_TEST

Suite* noop_suite(void) {
    Suite* s = suite_create("noop");
    TCase* tc = tcase_create("core");
    tcase_add_checked_fixture(tc, setup, fixture_teardown);

    tcase_add_test(tc, test_noop_always_keeps_the_lights);

    suite_add_tcase(s, tc);
    return s;
}
