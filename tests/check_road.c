#include <check.h>
#include <stddef.h>

#include "../src/game/entities/car.h"
#include "../src/game/entities/road.h"
#include "suites.h"

static road_t* road;

static void setup(void) {
    ck_assert_int_eq(road_create(&road), ROAD_STATUS_OK);
}

static void teardown(void) {
    if (road != NULL) {
        road_destroy(&road);
    }
}

static car_t* new_car(int x) {
    car_t* car = NULL;
    ck_assert_int_eq(car_create(x, 0, &car), CAR_STATUS_OK);
    return car;
}

START_TEST(test_road_starts_empty) {
    ck_assert_int_eq(road_size(road), 0);
}
END_TEST

START_TEST(test_road_append_and_get) {
    car_t* a = new_car(1);
    car_t* b = new_car(2);
    car_t* got = NULL;

    ck_assert_int_eq(road_append(road, a), ROAD_STATUS_OK);
    ck_assert_int_eq(road_append(road, b), ROAD_STATUS_OK);
    ck_assert_int_eq(road_size(road), 2);

    ck_assert_int_eq(road_get(road, 0, &got), ROAD_STATUS_OK);
    ck_assert_ptr_eq(got, a);
    ck_assert_int_eq(road_get(road, 1, &got), ROAD_STATUS_OK);
    ck_assert_ptr_eq(got, b);
}
END_TEST

START_TEST(test_road_append_full) {
    for (int i = 0; i < ROAD_MAX_SIZE; i++) {
        ck_assert_int_eq(road_append(road, new_car(i)), ROAD_STATUS_OK);
    }

    car_t* extra = new_car(-1);
    ck_assert_int_eq(road_append(road, extra), ROAD_STATUS_EFULL);
    ck_assert_int_eq(road_size(road), ROAD_MAX_SIZE);

    /* The road did not take ownership, so we free it. */
    car_destroy(extra);
}
END_TEST

START_TEST(test_road_get_invalid_index) {
    car_t* got = NULL;

    ck_assert_int_eq(road_get(road, 0, &got), ROAD_STATUS_EINDEX);
    ck_assert_int_eq(road_get(road, -1, &got), ROAD_STATUS_EINDEX);

    road_append(road, new_car(0));
    ck_assert_int_eq(road_get(road, 1, &got), ROAD_STATUS_EINDEX);
}
END_TEST

START_TEST(test_road_remove_shifts_remaining_cars) {
    car_t* a = new_car(1);
    car_t* b = new_car(2);
    car_t* c = new_car(3);
    car_t* got = NULL;

    road_append(road, a);
    road_append(road, b);
    road_append(road, c);

    ck_assert_int_eq(road_remove(road, 1), ROAD_STATUS_OK);
    ck_assert_int_eq(road_size(road), 2);

    road_get(road, 0, &got);
    ck_assert_ptr_eq(got, a);
    road_get(road, 1, &got);
    ck_assert_ptr_eq(got, c);
}
END_TEST

START_TEST(test_road_remove_invalid_index) {
    ck_assert_int_eq(road_remove(road, 0), ROAD_STATUS_EINDEX);
    ck_assert_int_eq(road_remove(road, -1), ROAD_STATUS_EINDEX);
}
END_TEST

START_TEST(test_road_destroy_nulls_pointer) {
    road_append(road, new_car(0));

    road_destroy(&road);

    ck_assert_ptr_null(road);
}
END_TEST

Suite* road_suite(void) {
    Suite* s = suite_create("road");
    TCase* tc = tcase_create("core");

    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_road_starts_empty);
    tcase_add_test(tc, test_road_append_and_get);
    tcase_add_test(tc, test_road_append_full);
    tcase_add_test(tc, test_road_get_invalid_index);
    tcase_add_test(tc, test_road_remove_shifts_remaining_cars);
    tcase_add_test(tc, test_road_remove_invalid_index);
    tcase_add_test(tc, test_road_destroy_nulls_pointer);

    suite_add_tcase(s, tc);
    return s;
}
