#include <check.h>
#include <stddef.h>

#include "../src/game/entities/car.h"
#include "suites.h"

START_TEST(test_car_create_sets_initial_state) {
    car_t* car = NULL;

    ck_assert_int_eq(car_create(3, 7, &car), CAR_STATUS_OK);
    ck_assert_ptr_nonnull(car);
    ck_assert_int_eq(car_get_x(car), 3);
    ck_assert_int_eq(car_get_y(car), 7);

    car_destroy(car);
}
END_TEST

START_TEST(test_car_move_applies_delta) {
    car_t* car = NULL;
    car_create(0, 0, &car);

    car_move(car, 2, -3);
    car_move(car, 1, 1);

    ck_assert_int_eq(car_get_x(car), 3);
    ck_assert_int_eq(car_get_y(car), -2);

    car_destroy(car);
}
END_TEST

START_TEST(test_car_null_safe_functions) {
    /* These must not crash; the test fails if the process dies. */
    car_wait(NULL);
    car_move(NULL, 1, 1);
    car_destroy(NULL);
}
END_TEST

Suite* car_suite(void) {
    Suite* s = suite_create("car");
    TCase* tc = tcase_create("core");

    tcase_add_test(tc, test_car_create_sets_initial_state);
    tcase_add_test(tc, test_car_move_applies_delta);
    tcase_add_test(tc, test_car_null_safe_functions);

    suite_add_tcase(s, tc);
    return s;
}
