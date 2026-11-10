#include <check.h>
#include <stddef.h>

#include "../src/game/entities/traffic_light.h"
#include "suites.h"

static traffic_light_t* light;

static void setup(void) {
    ck_assert_int_eq(traffic_light_create(2, 3, TRAFFIC_LIGHT_RED, &light), TRAFFIC_LIGHT_STATUS_OK);
}

static void teardown(void) {
    traffic_light_destroy(&light);
}

START_TEST(test_traffic_light_create_sets_initial_state) {
    ck_assert_ptr_nonnull(light);
    ck_assert_int_eq(traffic_light_get_x(light), 2);
    ck_assert_int_eq(traffic_light_get_y(light), 3);
    ck_assert_int_eq(traffic_light_get_color(light), TRAFFIC_LIGHT_RED);
}
END_TEST

START_TEST(test_traffic_light_toggle_flips_the_color) {
    traffic_light_toggle(light);
    ck_assert_int_eq(traffic_light_get_color(light), TRAFFIC_LIGHT_GREEN);

    traffic_light_toggle(light);
    ck_assert_int_eq(traffic_light_get_color(light), TRAFFIC_LIGHT_RED);
}
END_TEST

START_TEST(test_traffic_light_toggle_keeps_the_stop_cell) {
    traffic_light_toggle(light);

    ck_assert_int_eq(traffic_light_get_x(light), 2);
    ck_assert_int_eq(traffic_light_get_y(light), 3);
}
END_TEST

START_TEST(test_traffic_light_red_holds_the_stop_cell) {
    ck_assert(traffic_light_holds(light, 2, 3));
}
END_TEST

START_TEST(test_traffic_light_green_holds_nothing) {
    traffic_light_toggle(light);

    ck_assert(!traffic_light_holds(light, 2, 3));
}
END_TEST

START_TEST(test_traffic_light_red_ignores_other_cells) {
    /* x and y are swapped in the first one: a light that mixed them up would hold it. */
    ck_assert(!traffic_light_holds(light, 3, 2));
    ck_assert(!traffic_light_holds(light, 1, 3));
    ck_assert(!traffic_light_holds(light, 2, 4));
}
END_TEST

START_TEST(test_traffic_light_destroy_nulls_pointer) {
    traffic_light_destroy(&light);

    ck_assert_ptr_null(light);
}
END_TEST

START_TEST(test_traffic_light_null_safe_functions) {
    traffic_light_t* none = NULL;

    traffic_light_toggle(NULL);
    traffic_light_destroy(&none);
    traffic_light_destroy(NULL);
    ck_assert(!traffic_light_holds(NULL, 0, 0));
}
END_TEST

Suite* traffic_light_suite(void) {
    Suite* s = suite_create("traffic_light");
    TCase* tc = tcase_create("core");

    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_traffic_light_create_sets_initial_state);
    tcase_add_test(tc, test_traffic_light_toggle_flips_the_color);
    tcase_add_test(tc, test_traffic_light_toggle_keeps_the_stop_cell);
    tcase_add_test(tc, test_traffic_light_red_holds_the_stop_cell);
    tcase_add_test(tc, test_traffic_light_green_holds_nothing);
    tcase_add_test(tc, test_traffic_light_red_ignores_other_cells);
    tcase_add_test(tc, test_traffic_light_destroy_nulls_pointer);
    tcase_add_test(tc, test_traffic_light_null_safe_functions);

    suite_add_tcase(s, tc);
    return s;
}
