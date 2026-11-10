#define _POSIX_C_SOURCE 200809L /* open_memstream is hidden under strict -std=c2x */

#include <check.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game/entities/car.h"
#include "../src/game/entities/road.h"
#include "../src/game/entities/traffic_light.h"
#include "../src/game/view.h"
#include "suites.h"

/* Not square, so a width/height mix-up in the view shows up as a failure. */
#define WIDTH 7
#define HEIGHT 5

/* A line of the frame: the cells, then VIEW_EDGE, then the newline. */
#define LINE (WIDTH + 2)

static road_t* horizontal;
static road_t* vertical;
static view_t* view;
static FILE* out;
static char* output;
static size_t output_size;

/* Lights made by make_light(), freed in teardown. */
static traffic_light_t* lights[2];
static int light_count;

static void setup(void) {
    ck_assert_int_eq(road_create(&horizontal), ROAD_STATUS_OK);
    ck_assert_int_eq(road_create(&vertical), ROAD_STATUS_OK);

    out = open_memstream(&output, &output_size);
    ck_assert_ptr_nonnull(out);
    ck_assert_int_eq(view_create(out, WIDTH, HEIGHT, &view), VIEW_STATUS_OK);
}

static void teardown(void) {
    view_destroy(&view);
    fclose(out);
    free(output);
    output = NULL;

    road_destroy(&horizontal);
    road_destroy(&vertical);

    for (int i = 0; i < light_count; i++) {
        traffic_light_destroy(&lights[i]);
    }
    light_count = 0;
}

static traffic_light_t* make_light(int x, int y, traffic_light_color_t color) {
    ck_assert_int_lt(light_count, 2);
    traffic_light_t** light = &lights[light_count++];
    ck_assert_int_eq(traffic_light_create(x, y, color, light), TRAFFIC_LIGHT_STATUS_OK);
    return *light;
}

static void add_car(road_t* road, int x, int y) {
    car_t* car = NULL;
    ck_assert_int_eq(car_create(x, y, &car), CAR_STATUS_OK);
    ck_assert_int_eq(road_append(road, car), ROAD_STATUS_OK);
}

/** Renders a frame with roads and lights and returns it without the clear-screen prefix. */
static const char* render_all(const road_t* h, const road_t* v, const traffic_light_t* hl, const traffic_light_t* vl) {
    rewind(out);
    view_render(view, h, v, hl, vl);

    size_t prefix = strlen(VIEW_CLEAR_SCREEN);
    ck_assert_uint_eq(output_size, prefix + HEIGHT * LINE);
    ck_assert_int_eq(strncmp(output, VIEW_CLEAR_SCREEN, prefix), 0);

    return output + prefix;
}

/** Renders a frame with only the roads. */
static const char* render(const road_t* h, const road_t* v) {
    return render_all(h, v, NULL, NULL);
}

/** Renders a frame with only the lights. */
static const char* render_lights(const traffic_light_t* hl, const traffic_light_t* vl) {
    return render_all(NULL, NULL, hl, vl);
}

/** The character drawn at (x, y): column x of line y. */
static char cell_at(const char* frame, int x, int y) {
    return frame[y * LINE + x];
}

/** True when every cell of the frame is empty (and every line ends with the edge). */
static bool is_blank(const char* frame) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (cell_at(frame, x, y) != VIEW_EMPTY) {
                return false;
            }
        }

        ck_assert_int_eq(cell_at(frame, WIDTH, y), VIEW_EDGE);
        ck_assert_int_eq(cell_at(frame, WIDTH + 1, y), '\n');
    }

    return true;
}

START_TEST(test_view_frame_has_the_configured_size) {
    /* render() checks the total length: HEIGHT lines of WIDTH cells + edge + newline. */
    ck_assert(is_blank(render(horizontal, vertical)));
}
END_TEST

START_TEST(test_view_null_roads_draw_blank_frame) {
    ck_assert(is_blank(render(NULL, NULL)));
}
END_TEST

START_TEST(test_view_draws_each_road_with_its_symbol) {
    add_car(horizontal, 3, 1);
    add_car(vertical, 5, 4);

    const char* frame = render(horizontal, vertical);

    ck_assert_int_eq(cell_at(frame, 3, 1), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, 5, 4), VIEW_CAR_VERTICAL);
}
END_TEST

START_TEST(test_view_draws_every_car) {
    add_car(horizontal, 2, 1);
    add_car(horizontal, 6, 1);
    add_car(vertical, 4, 3);

    const char* frame = render(horizontal, vertical);

    ck_assert_int_eq(cell_at(frame, 2, 1), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, 6, 1), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, 4, 3), VIEW_CAR_VERTICAL);
    ck_assert_int_eq(cell_at(frame, 0, 0), VIEW_EMPTY);
}
END_TEST

START_TEST(test_view_origin_is_top_left) {
    add_car(horizontal, 0, 0);

    /* (0, 0) is the very first character of the frame. */
    ck_assert_int_eq(render(horizontal, vertical)[0], VIEW_CAR_HORIZONTAL);
}
END_TEST

START_TEST(test_view_y_axis_points_down_up_to_height) {
    add_car(horizontal, 0, 0);
    add_car(vertical, 0, HEIGHT - 1);

    const char* frame = render(horizontal, vertical);

    /* y = 0 is the first line and y = HEIGHT - 1 is the last. */
    ck_assert_int_eq(cell_at(frame, 0, 0), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, 0, HEIGHT - 1), VIEW_CAR_VERTICAL);
}
END_TEST

START_TEST(test_view_x_axis_points_right_up_to_width) {
    add_car(horizontal, 0, 2);
    add_car(vertical, WIDTH - 1, 2);

    const char* frame = render(horizontal, vertical);

    /* x = 0 is the first column and x = WIDTH - 1 is the last, right before the edge. */
    ck_assert_int_eq(cell_at(frame, 0, 2), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, WIDTH - 1, 2), VIEW_CAR_VERTICAL);
    ck_assert_int_eq(cell_at(frame, WIDTH, 2), VIEW_EDGE);
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
    add_car(horizontal, WIDTH, 0);
    add_car(horizontal, -1, 0);
    add_car(vertical, 0, HEIGHT);
    add_car(vertical, 0, -1);
    /* Within the width but past the height: a view that mixed the two up would draw this. */
    add_car(horizontal, 0, WIDTH - 1);

    ck_assert(is_blank(render(horizontal, vertical)));
}
END_TEST

START_TEST(test_view_render_twice_does_not_keep_the_old_frame) {
    add_car(horizontal, 1, 1);
    render(horizontal, vertical);

    ck_assert(is_blank(render(NULL, NULL)));
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

START_TEST(test_view_horizontal_light_is_drawn_below_its_stop_cell) {
    traffic_light_t* light = make_light(3, 1, TRAFFIC_LIGHT_GREEN);

    const char* frame = render_lights(light, NULL);

    ck_assert_int_eq(cell_at(frame, 3, 2), VIEW_LIGHT_GREEN);
    ck_assert_int_eq(cell_at(frame, 3, 1), VIEW_EMPTY);
}
END_TEST

START_TEST(test_view_vertical_light_is_drawn_left_of_its_stop_cell) {
    traffic_light_t* light = make_light(3, 1, TRAFFIC_LIGHT_RED);

    const char* frame = render_lights(NULL, light);

    ck_assert_int_eq(cell_at(frame, 2, 1), VIEW_LIGHT_RED);
    ck_assert_int_eq(cell_at(frame, 3, 1), VIEW_EMPTY);
}
END_TEST

START_TEST(test_view_light_symbol_follows_its_color) {
    traffic_light_t* green = make_light(1, 1, TRAFFIC_LIGHT_GREEN);
    traffic_light_t* red = make_light(5, 1, TRAFFIC_LIGHT_RED);

    const char* frame = render_lights(green, red);

    ck_assert_int_eq(cell_at(frame, 1, 2), VIEW_LIGHT_GREEN);
    ck_assert_int_eq(cell_at(frame, 4, 1), VIEW_LIGHT_RED);
}
END_TEST

START_TEST(test_view_lights_next_to_cars_leave_the_cars_alone) {
    /* The layout the game uses: the roads cross at (4, 2), each stop cell holds a waiting car. */
    traffic_light_t* hl = make_light(3, 2, TRAFFIC_LIGHT_RED);
    traffic_light_t* vl = make_light(4, 1, TRAFFIC_LIGHT_RED);
    add_car(horizontal, 3, 2);
    add_car(vertical, 4, 1);

    const char* frame = render_all(horizontal, vertical, hl, vl);

    ck_assert_int_eq(cell_at(frame, 3, 2), VIEW_CAR_HORIZONTAL);
    ck_assert_int_eq(cell_at(frame, 4, 1), VIEW_CAR_VERTICAL);
    ck_assert_int_eq(cell_at(frame, 3, 3), VIEW_LIGHT_RED);
    ck_assert_int_eq(cell_at(frame, 3, 1), VIEW_LIGHT_RED);
}
END_TEST

START_TEST(test_view_lights_outside_the_grid_are_not_drawn) {
    /* Each stop cell is in the grid, but the light's own cell is not. */
    traffic_light_t* hl = make_light(0, HEIGHT - 1, TRAFFIC_LIGHT_GREEN);
    traffic_light_t* vl = make_light(0, 0, TRAFFIC_LIGHT_RED);

    ck_assert(is_blank(render_lights(hl, vl)));
}
END_TEST

START_TEST(test_view_create_rejects_zero_size) {
    view_t* other = NULL;

    ck_assert_int_eq(view_create(out, 0, HEIGHT, &other), VIEW_STATUS_EINVAL);
    ck_assert_int_eq(view_create(out, WIDTH, 0, &other), VIEW_STATUS_EINVAL);
    ck_assert_ptr_null(other);
}
END_TEST

START_TEST(test_view_create_rejects_size_that_overflows) {
    view_t* other = NULL;

    ck_assert_int_eq(view_create(out, UINT_MAX, 2, &other), VIEW_STATUS_ENOMEM);
    ck_assert_int_eq(view_create(out, 2, UINT_MAX, &other), VIEW_STATUS_ENOMEM);
    ck_assert_ptr_null(other);
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
    tcase_add_test(tc, test_view_frame_has_the_configured_size);
    tcase_add_test(tc, test_view_null_roads_draw_blank_frame);
    tcase_add_test(tc, test_view_draws_each_road_with_its_symbol);
    tcase_add_test(tc, test_view_draws_every_car);
    tcase_add_test(tc, test_view_origin_is_top_left);
    tcase_add_test(tc, test_view_y_axis_points_down_up_to_height);
    tcase_add_test(tc, test_view_x_axis_points_right_up_to_width);
    tcase_add_test(tc, test_view_cars_from_both_roads_in_one_cell_collide);
    tcase_add_test(tc, test_view_cars_of_one_road_in_one_cell_do_not_collide);
    tcase_add_test(tc, test_view_cars_outside_the_grid_are_not_drawn);
    tcase_add_test(tc, test_view_render_twice_does_not_keep_the_old_frame);
    tcase_add_test(tc, test_view_render_does_not_change_the_roads);
    tcase_add_test(tc, test_view_horizontal_light_is_drawn_below_its_stop_cell);
    tcase_add_test(tc, test_view_vertical_light_is_drawn_left_of_its_stop_cell);
    tcase_add_test(tc, test_view_light_symbol_follows_its_color);
    tcase_add_test(tc, test_view_lights_next_to_cars_leave_the_cars_alone);
    tcase_add_test(tc, test_view_lights_outside_the_grid_are_not_drawn);
    tcase_add_test(tc, test_view_create_rejects_zero_size);
    tcase_add_test(tc, test_view_create_rejects_size_that_overflows);
    tcase_add_test(tc, test_view_destroy_nulls_pointer);
    tcase_add_test(tc, test_view_destroy_null_safe);

    suite_add_tcase(s, tc);
    return s;
}
