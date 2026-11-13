#include "ai_fixture.h"
#include "suites.h"

static void setup(void) {
    fixture_setup("agent=noop\n");
}

START_TEST(test_noop_always_decides_noop) {
    ck_assert_int_eq(ai_create(config, &ai), AI_STATUS_OK);

    for (int tick = 0; tick < 50; tick++) {
        game_command_t command = ai_decide(ai, game);
        ck_assert_int_eq(command, GAME_COMMAND_NOOP);
        game_exec(game, command);
    }
}
END_TEST

Suite* noop_suite(void) {
    Suite* s = suite_create("noop");
    TCase* tc = tcase_create("core");
    tcase_add_checked_fixture(tc, setup, fixture_teardown);

    tcase_add_test(tc, test_noop_always_decides_noop);

    suite_add_tcase(s, tc);
    return s;
}
