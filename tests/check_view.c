#define _POSIX_C_SOURCE 200809L /* open_memstream is hidden under strict -std=c2x */

#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game/entities/car.h"
#include "../src/game/entities/road.h"
#include "../src/game/view.h"
#include "suites.h"

static road_t* horizontal;
static road_t* vertical;
static view_t* view;
static FILE* out;
static char* output;
static size_t output_size;

static void setup(void) {
    ck_assert_int_eq(road_create(&horizontal), ROAD_STATUS_OK);
    ck_assert_int_eq(road_create(&vertical), ROAD_STATUS_OK);

    out = open_memstream(&output, &output_size);
    ck_assert_ptr_nonnull(out);
    ck_assert_int_eq(view_create(out, &view), VIEW_STATUS_OK);
}

static void teardown(void) {
    view_destroy(&view);
    fclose(out);
    free(output);
    output = NULL;

    road_destroy(&horizontal);
    road_destroy(&vertical);
}

static void add_car(road_t* road, int x, int y) {
    car_t* car = NULL;
    ck_assert_int_eq(car_create(x, y, &car), CAR_STATUS_OK);
    ck_assert_int_eq(road_append(road, car), ROAD_STATUS_OK);
}

/** Renders a frame and returns it without the clear-screen prefix. */
static const char* render(const road_t* h, const road_t* v) {
    rewind(out);
    view_render(view, h, v);

    size_t prefix = strlen(VIEW_CLEAR_SCREEN);
    ck_assert_uint_eq(output_size, prefix + VIEW_SIZE * (VIEW_SIZE + 1));
    ck_assert_int_eq(strncmp(output, VIEW_CLEAR_SCREEN, prefix), 0);

    return output + prefix;
}

/** The character drawn for the world position (x, y). */
static char cell_at(const char* frame, int x, int y) {
    return frame[(VIEW_RADIUS - y) * (VIEW_SIZE + 1) + (x + VIEW_RADIUS)];
}

/** True when every cell of the frame is empty. */
static bool is_blank(const char* frame) {
    for (int row = 0; row < VIEW_SIZE; row++) {
        for (int col = 0; col < VIEW_SIZE; col++) {
            if (frame[row * (VIEW_SIZE + 1) + col] != VIEW_EMPTY) {
                return false;
            }
        }
        ck_assert_int_eq(frame[row * (VIEW_SIZE + 1) + VIEW_SIZE], '\n');
    }

    return true;
}

START_TEST(test_view_empty_roads_draw_blank_frame) {
    ck_assert(is_blank(render(horizontal, vertical)));
}
END_TEST

START_TEST(test_view_null_roads_draw_blank_frame) {
    ck_assert(is_blank(render(NULL, NULL)));
}
END_TEST

START_TEST(test_view_draws_each_road_with_its_symbol) {
    add_car(horizontal, 0, 0);
    add_car(vertical, -3, 4);

    const char* frame = render(horizontal, vertical);

    ck_assert_int_eq(cell_at(frame, 0, 0), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, -3, 4), VIEW_CAR_VERTICAL);
}
END_TEST

START_TEST(test_view_draws_every_car) {
    add_car(horizontal, -2, 1);
    add_car(horizontal, 5, 1);
    add_car(vertical, 1, -6);

    const char* frame = render(horizontal, vertical);

    ck_assert_int_eq(cell_at(frame, -2, 1), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, 5, 1), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, 1, -6), VIEW_CAR_VERTICAL);
    ck_assert_int_eq(cell_at(frame, 0, 0), VIEW_EMPTY);
}
END_TEST

START_TEST(test_view_y_axis_points_up) {
    add_car(horizontal, 0, VIEW_RADIUS);
    add_car(vertical, 0, -VIEW_RADIUS);

    const char* frame = render(horizontal, vertical);

    /* The first line of the frame is the top row (y = +radius), the last is the bottom. */
    ck_assert_int_eq(frame[VIEW_RADIUS], VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(frame[(VIEW_SIZE - 1) * (VIEW_SIZE + 1) + VIEW_RADIUS], VIEW_CAR_VERTICAL);
}
END_TEST

START_TEST(test_view_x_axis_points_right) {
    add_car(horizontal, -VIEW_RADIUS, 0);
    add_car(vertical, VIEW_RADIUS, 0);

    const char* frame = render(horizontal, vertical);
    const char* middle_row = frame + VIEW_RADIUS * (VIEW_SIZE + 1);

    ck_assert_int_eq(middle_row[0], VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(middle_row[VIEW_SIZE - 1], VIEW_CAR_VERTICAL);
}
END_TEST

START_TEST(test_view_cars_from_both_roads_in_one_cell_collide) {
    add_car(horizontal, 2, 2);
    add_car(vertical, 2, 2);

    ck_assert_int_eq(cell_at(render(horizontal, vertical), 2, 2), VIEW_COLLISION);
}
END_TEST

START_TEST(test_view_cars_of_one_road_in_one_cell_do_not_collide) {
    add_car(horizontal, 2, 2);
    add_car(horizontal, 2, 2);

    ck_assert_int_eq(cell_at(render(horizontal, vertical), 2, 2), VIEW_CAR_HORIZONTAL);
}
END_TEST

START_TEST(test_view_cars_outside_the_grid_are_not_drawn) {
    add_car(horizontal, VIEW_RADIUS + 1, 0);
    add_car(horizontal, -VIEW_RADIUS - 1, 0);
    add_car(vertical, 0, VIEW_RADIUS + 1);
    add_car(vertical, 0, -VIEW_RADIUS - 1);

    ck_assert(is_blank(render(horizontal, vertical)));
}
END_TEST

START_TEST(test_view_render_does_not_change_the_roads) {
    add_car(horizontal, 1, 1);
    add_car(vertical, 2, 2);

    render(horizontal, vertical);

    ck_assert_int_eq(road_size(horizontal), 1);
    ck_assert_int_eq(road_size(vertical), 1);
}
END_TEST

START_TEST(test_view_destroy_nulls_pointer) {
    view_destroy(&view);

    ck_assert_ptr_null(view);
}
END_TEST

START_TEST(test_view_destroy_null_safe) {
    view_t* none = NULL;

    view_destroy(&none);
    view_destroy(NULL);
}
END_TEST

Suite* view_suite(void) {
    Suite* s = suite_create("view");
    TCase* tc = tcase_create("core");

    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_view_empty_roads_draw_blank_frame);
    tcase_add_test(tc, test_view_null_roads_draw_blank_frame);
    tcase_add_test(tc, test_view_draws_each_road_with_its_symbol);
    tcase_add_test(tc, test_view_draws_every_car);
    tcase_add_test(tc, test_view_y_axis_points_up);
    tcase_add_test(tc, test_view_x_axis_points_right);
    tcase_add_test(tc, test_view_cars_from_both_roads_in_one_cell_collide);
    tcase_add_test(tc, test_view_cars_of_one_road_in_one_cell_do_not_collide);
    tcase_add_test(tc, test_view_cars_outside_the_grid_are_not_drawn);
    tcase_add_test(tc, test_view_render_does_not_change_the_roads);
    tcase_add_test(tc, test_view_destroy_nulls_pointer);
    tcase_add_test(tc, test_view_destroy_null_safe);

    suite_add_tcase(s, tc);
    return s;
}
