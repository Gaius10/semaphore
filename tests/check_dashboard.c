#define _POSIX_C_SOURCE 200809L /* open_memstream and setenv are hidden under strict -std=c2x */

#include <check.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/training/dashboard.h"
#include "suites.h"

/** The pure formatting functions, then the dashboard itself writing to memory or to a file. */

#define BLOCK_LOW "▁"
#define BLOCK_MID "▄"
#define BLOCK_FULL "█"
#define EMPTY_CELL "░"

static unsigned int count_of(const char* text, const char* piece) {
    unsigned int count = 0;
    for (const char* at = strstr(text, piece); at != NULL; at = strstr(at + strlen(piece), piece)) {
        count++;
    }
    return count;
}

/* ---------------------------------------------------------------- pure helpers */

START_TEST(test_dashboard_bar_fills_in_proportion_and_a_full_bar_means_done) {
    char bar[128];

    ck_assert_uint_eq(dashboard_bar(bar, sizeof(bar), 0.0, 10), 30);
    ck_assert_uint_eq(count_of(bar, BLOCK_FULL), 0);
    ck_assert_uint_eq(count_of(bar, EMPTY_CELL), 10);

    dashboard_bar(bar, sizeof(bar), 0.5, 10);
    ck_assert_uint_eq(count_of(bar, BLOCK_FULL), 5);
    ck_assert_uint_eq(count_of(bar, EMPTY_CELL), 5);

    dashboard_bar(bar, sizeof(bar), 0.99, 10); /* rounded down: not done yet */
    ck_assert_uint_eq(count_of(bar, BLOCK_FULL), 9);

    dashboard_bar(bar, sizeof(bar), 1.0, 10);
    ck_assert_uint_eq(count_of(bar, BLOCK_FULL), 10);

    dashboard_bar(bar, sizeof(bar), 3.7, 4); /* clamped */
    ck_assert_uint_eq(count_of(bar, BLOCK_FULL), 4);
    dashboard_bar(bar, sizeof(bar), -2.0, 4);
    ck_assert_uint_eq(count_of(bar, BLOCK_FULL), 0);
    dashboard_bar(bar, sizeof(bar), NAN, 4);
    ck_assert_uint_eq(count_of(bar, EMPTY_CELL), 4);
}
END_TEST

START_TEST(test_dashboard_bar_that_does_not_fit_is_empty) {
    char small[10];

    ck_assert_uint_eq(dashboard_bar(small, sizeof(small), 0.5, 10), 0);
    ck_assert_str_eq(small, "");
    ck_assert_uint_eq(dashboard_bar(small, sizeof(small), 0.5, 3), 9);
}
END_TEST

START_TEST(test_dashboard_sparkline_goes_from_the_lowest_to_the_highest_block) {
    char line[128];
    double rising[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    dashboard_sparkline(line, sizeof(line), rising, 8, 8);
    ck_assert_str_eq(line, "▁▂▃▄▅▆▇█");

    double falling[] = { 10, 0 };
    dashboard_sparkline(line, sizeof(line), falling, 2, 5);
    ck_assert_str_eq(line, "█▁");

    double negative[] = { -5, -1 };
    dashboard_sparkline(line, sizeof(line), negative, 2, 5);
    ck_assert_str_eq(line, "▁█");
}
END_TEST

START_TEST(test_dashboard_sparkline_shows_only_the_last_values_and_handles_flat_and_empty_series) {
    char line[128];
    double values[] = { 100, 1, 2, 3 };

    /* Only the last 3 count, so the 100 does not squash the rest. */
    dashboard_sparkline(line, sizeof(line), values, 4, 3);
    ck_assert_str_eq(line, "▁▅█");

    double flat[] = { 7, 7, 7 };
    dashboard_sparkline(line, sizeof(line), flat, 3, 10);
    ck_assert_str_eq(line, BLOCK_MID BLOCK_MID BLOCK_MID);

    ck_assert_uint_eq(dashboard_sparkline(line, sizeof(line), values, 0, 10), 0);
    ck_assert_str_eq(line, "");

    char small[4];
    ck_assert_uint_eq(dashboard_sparkline(small, sizeof(small), values, 4, 4), 0); /* 12 bytes needed */
}
END_TEST

START_TEST(test_dashboard_levels_scale_from_zero_to_the_given_maximum) {
    char line[128];
    double genes[] = { 0.0, 1.0, 2.0, 3.0 };

    dashboard_levels(line, sizeof(line), genes, 4, 2.0);
    ck_assert_str_eq(line, BLOCK_LOW "▅" BLOCK_FULL BLOCK_FULL); /* 3.0 is clamped to full */

    double odd[] = { -1.0, NAN };
    dashboard_levels(line, sizeof(line), odd, 2, 2.0);
    ck_assert_str_eq(line, BLOCK_LOW BLOCK_LOW);

    dashboard_levels(line, sizeof(line), genes, 4, 0.0); /* no range: all low, no division by zero */
    ck_assert_str_eq(line, BLOCK_LOW BLOCK_LOW BLOCK_LOW BLOCK_LOW);
}
END_TEST

START_TEST(test_dashboard_delta_shows_the_direction_and_ignores_tiny_changes) {
    char text[64];

    dashboard_format_delta(text, sizeof(text), 1.333);
    ck_assert_str_eq(text, "▲ +1.33");
    dashboard_format_delta(text, sizeof(text), -0.5);
    ck_assert_str_eq(text, "▼ -0.50");
    dashboard_format_delta(text, sizeof(text), 0.004);
    ck_assert_str_eq(text, "= 0.00");
    dashboard_format_delta(text, sizeof(text), -0.004);
    ck_assert_str_eq(text, "= 0.00");
    dashboard_format_delta(text, sizeof(text), NAN);
    ck_assert_str_eq(text, "= 0.00");
}
END_TEST

START_TEST(test_dashboard_duration_uses_the_unit_that_fits) {
    char text[32];
    struct {
        double seconds;
        const char* expected;
    } cases[] = {
        { 0.0, "0.0s" }, { 12.34, "12.3s" }, { 59.9, "59.9s" }, { 60.0, "1m00s" }, { 65.2, "1m05s" },
        { 3599.0, "59m59s" }, { 3600.0, "1h00m" }, { 3725.0, "1h02m" }, { -1.0, "--" }, { NAN, "--" },
    };

    for (unsigned int i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        dashboard_format_duration(text, sizeof(text), cases[i].seconds);
        ck_assert_str_eq(text, cases[i].expected);
    }
}
END_TEST

START_TEST(test_dashboard_chooses_the_live_style_only_on_a_big_enough_terminal) {
    unsigned int height = dashboard_frame_height(4, 100);

    ck_assert_int_eq(dashboard_choose_style(true, false, 100, height + 1, 4), DASHBOARD_LIVE);
    ck_assert_int_eq(dashboard_choose_style(false, false, 100, 100, 4), DASHBOARD_PLAIN); /* not a terminal */
    ck_assert_int_eq(dashboard_choose_style(true, true, 100, 100, 4), DASHBOARD_PLAIN);   /* TERM=dumb */
    ck_assert_int_eq(dashboard_choose_style(true, false, DASHBOARD_MIN_WIDTH - 1, 100, 4), DASHBOARD_PLAIN);
    ck_assert_int_eq(dashboard_choose_style(true, false, DASHBOARD_MIN_WIDTH, 100, 4), DASHBOARD_LIVE);
    ck_assert_int_eq(dashboard_choose_style(true, false, 100, height, 4), DASHBOARD_PLAIN); /* no spare line */

    /* More workers need more lines. */
    ck_assert_uint_gt(dashboard_frame_height(32, 80), dashboard_frame_height(4, 80));
    ck_assert_int_eq(dashboard_choose_style(true, false, 80, dashboard_frame_height(4, 80) + 1, 64), DASHBOARD_PLAIN);
}
END_TEST

/* ---------------------------------------------------------------- the dashboard */

static dashboard_info_t info_for(unsigned int workers, unsigned int generations, const char* csv_path) {
    return (dashboard_info_t) {
        .population_size = 6,
        .generations = generations,
        .episodes_per_candidate = 2,
        .episode_ticks = 80,
        .workers = workers,
        .seed = 7,
        .length_h = 10,
        .length_v = 12,
        .coeficient_max = 2.0,
        .threshold_max = 4.0,
        .csv_path = csv_path,
    };
}

/** A generation whose champion has `fitness`, with a made-up genome of the right length. */
static dashboard_generation_t generation_with(unsigned int index, double fitness, const double* genome) {
    return (dashboard_generation_t) {
        .index = index,
        .champion_fitness = fitness,
        .mean_fitness = fitness - 10.0,
        .worst_fitness = fitness - 20.0,
        .champion_cars_passed = fitness,
        .champion_crashes = 0.0,
        .champion_genome = genome,
    };
}

/** Everything written to the memory stream so far. The caller frees it. */
static char* written(FILE* stream, char** buffer) {
    fflush(stream);
    return *buffer;
}

/** Display columns of a line, ignoring the ANSI escapes and counting each UTF-8 character as one. */
static unsigned int display_width(const char* line, size_t length) {
    unsigned int columns = 0;
    for (size_t i = 0; i < length; i++) {
        if (line[i] == '\x1b') {
            while (i < length && !(line[i] >= 'A' && line[i] <= 'Z') && !(line[i] >= 'a' && line[i] <= 'z')) {
                i++;
            }
            continue;
        }
        if (((unsigned char)line[i] & 0xC0) != 0x80) {
            columns++;
        }
    }
    return columns;
}

START_TEST(test_dashboard_live_panel_has_the_announced_height_and_fits_the_width) {
    setenv("NO_COLOR", "1", 1);
    unsigned int widths[] = { DASHBOARD_MIN_WIDTH, 80, 100 };
    unsigned int worker_counts[] = { 1, 4, 16, 17, 40 };

    for (unsigned int a = 0; a < 3; a++) {
        for (unsigned int b = 0; b < 5; b++) {
            char* buffer = NULL;
            size_t size = 0;
            FILE* stream = open_memstream(&buffer, &size);
            ck_assert_ptr_nonnull(stream);

            /* A long path and long roads: the panel must still fit in the width. */
            char long_path[300];
            memset(long_path, 'p', sizeof(long_path) - 1);
            long_path[sizeof(long_path) - 1] = '\0';
            dashboard_info_t info = info_for(worker_counts[b], 1000, long_path);
            info.length_h = 150;
            info.length_v = 200;
            info.population_size = 100000;
            dashboard_t* dashboard = NULL;
            ck_assert(dashboard_create(stream, DASHBOARD_LIVE, widths[a], &info, &dashboard));

            double genome[1 + 150 + 200];
            for (int g = 0; g < 351; g++) {
                genome[g] = 0.01 * g;
            }
            dashboard_generation_t first = generation_with(0, 1234567.891, genome);
            dashboard_add_generation(dashboard, &first);
            dashboard_finish(dashboard, true);

            /* Walk the first frame, the one drawn when it was created: it ends where the next draw
             * starts, with "ESC [ <height> A" and a carriage return (the frame itself has none). */
            fflush(stream);
            const char* frame = strstr(buffer, "\x1b[?25l") + strlen("\x1b[?25l");
            const char* frame_end = strchr(frame, '\r');
            ck_assert_ptr_nonnull(frame_end);
            unsigned int lines = 0;
            const char* line = frame;
            for (const char* at = frame; at < frame_end; at++) {
                if (*at == '\n') {
                    ck_assert_msg(
                        display_width(line, (size_t)(at - line)) <= widths[a],
                        "width %u, workers %u, line %u is %u columns wide",
                        widths[a], worker_counts[b], lines, display_width(line, (size_t)(at - line))
                    );
                    lines++;
                    line = at + 1;
                }
            }
            ck_assert_uint_eq(lines, dashboard_frame_height(worker_counts[b], widths[a]));

            dashboard_destroy(&dashboard);
            fclose(stream);
            free(buffer);
        }
    }
}
END_TEST

START_TEST(test_dashboard_live_hides_the_cursor_redraws_in_place_and_shows_it_again) {
    setenv("NO_COLOR", "1", 1);
    char* buffer = NULL;
    size_t size = 0;
    FILE* stream = open_memstream(&buffer, &size);
    dashboard_info_t info = info_for(2, 4, "data/training_x.csv");
    dashboard_t* dashboard = NULL;
    ck_assert(dashboard_create(stream, DASHBOARD_LIVE, 80, &info, &dashboard));

    char* text = written(stream, &buffer);
    ck_assert(strncmp(text, "\x1b[?25l", 6) == 0); /* the cursor is hidden first... */
    ck_assert_ptr_null(strstr(text, "\x1b[?25h"));  /* ...and not shown until the end */
    ck_assert_ptr_nonnull(strstr(text, "Generation 1/4"));
    ck_assert_ptr_nonnull(strstr(text, "data/training_x.csv"));

    unsigned int up = dashboard_frame_height(2, 80);
    char move_up[16];
    snprintf(move_up, sizeof(move_up), "\x1b[%uA", up);
    ck_assert_uint_eq(count_of(text, move_up), 0); /* the first draw goes where the cursor is */

    /* Workers 0 and 1 have 3 candidates each; 2 and 1 are done. */
    unsigned int assigned[] = { 3, 3 };
    unsigned int finished[] = { 2, 1 };
    evaluator_progress_t progress = {
        .generation = 0, .total = 6, .done = 3, .workers = 2, .assigned = assigned, .finished = finished,
    };
    dashboard_update_progress(dashboard, &progress);
    text = written(stream, &buffer);
    ck_assert_uint_eq(count_of(text, move_up), 1); /* now it goes back up over the panel */
    ck_assert_ptr_nonnull(strstr(text, "3/6"));
    ck_assert_ptr_nonnull(strstr(text, "w00 [████░░]  2/3"));
    ck_assert_ptr_nonnull(strstr(text, "w01 [██░░░░]  1/3"));

    dashboard_finish(dashboard, true);
    text = written(stream, &buffer);
    ck_assert_ptr_nonnull(strstr(text, "100%"));
    const char* tail = text + strlen(text) - 6;
    ck_assert_str_eq(tail, "\x1b[?25h"); /* the cursor is back, below the last panel */

    dashboard_destroy(&dashboard);
    fclose(stream);
    free(buffer);
}
END_TEST

START_TEST(test_dashboard_live_shows_the_fitness_its_change_and_the_history) {
    setenv("NO_COLOR", "1", 1);
    char* buffer = NULL;
    size_t size = 0;
    FILE* stream = open_memstream(&buffer, &size);
    dashboard_info_t info = info_for(1, 5, "x.csv");
    dashboard_t* dashboard = NULL;
    ck_assert(dashboard_create(stream, DASHBOARD_LIVE, 80, &info, &dashboard));

    double genome[23];
    for (int g = 0; g < 23; g++) {
        genome[g] = g == 0 ? 2.0 : (g % 2 ? 2.0 : 0.0); /* threshold half of its range; genes alternate full and empty */
    }

    dashboard_generation_t first = generation_with(0, 100.0, genome);
    dashboard_add_generation(dashboard, &first);
    char* text = written(stream, &buffer);
    ck_assert_ptr_nonnull(strstr(text, "100.00"));
    ck_assert_ptr_null(strstr(text, "▲")); /* nothing to compare with yet */

    dashboard_generation_t second = generation_with(1, 110.5, genome);
    dashboard_add_generation(dashboard, &second);
    dashboard_generation_t third = generation_with(2, 105.0, genome);
    dashboard_add_generation(dashboard, &third);
    text = written(stream, &buffer);

    ck_assert_ptr_nonnull(strstr(text, "▲ +10.50")); /* generation 2 against generation 1 */
    ck_assert_ptr_nonnull(strstr(text, "▼ -5.50"));  /* generation 3 against generation 2 */
    ck_assert_ptr_nonnull(strstr(text, "best ever   110.50  (generation 2)"));
    ck_assert_ptr_nonnull(strstr(text, "cars 105.0 | crashes 0.00 | threshold 2.000"));
    ck_assert_ptr_nonnull(strstr(text, "horizontal  █▁█▁█▁█▁█▁")); /* the genes of the last champion */
    ck_assert_ptr_nonnull(strstr(text, "champion  ▁█▄"));        /* 100, 110.5, 105 */

    dashboard_destroy(&dashboard);
    fclose(stream);
    free(buffer);
}
END_TEST

START_TEST(test_dashboard_a_run_stopped_early_does_not_show_100_percent) {
    setenv("NO_COLOR", "1", 1);
    char* buffer = NULL;
    size_t size = 0;
    FILE* stream = open_memstream(&buffer, &size);
    dashboard_info_t info = info_for(1, 4, "x.csv");
    dashboard_t* dashboard = NULL;
    ck_assert(dashboard_create(stream, DASHBOARD_LIVE, 80, &info, &dashboard));

    double genome[23] = { 0 };
    dashboard_generation_t first = generation_with(0, 100.0, genome);
    dashboard_add_generation(dashboard, &first);
    dashboard_finish(dashboard, false);

    char* text = written(stream, &buffer);
    const char* last_panel = strrchr(text, 'A'); /* the last "ESC [ n A" that started a redraw */
    ck_assert_ptr_nonnull(last_panel);
    ck_assert_ptr_nonnull(strstr(last_panel, "Generation 1/4"));
    ck_assert_ptr_nonnull(strstr(last_panel, " 25%"));
    ck_assert_ptr_null(strstr(last_panel, "100%"));
    ck_assert_ptr_nonnull(strstr(last_panel, "left --"));

    dashboard_destroy(&dashboard);
    fclose(stream);
    free(buffer);
}
END_TEST

START_TEST(test_dashboard_plain_prints_a_summary_and_one_line_per_generation_without_escapes) {
    char* buffer = NULL;
    size_t size = 0;
    FILE* stream = open_memstream(&buffer, &size);
    dashboard_info_t info = info_for(3, 12, "data/training_x.csv");
    dashboard_t* dashboard = NULL;
    ck_assert(dashboard_create(stream, DASHBOARD_PLAIN, 0, &info, &dashboard));

    unsigned int assigned[] = { 2, 2, 2 };
    unsigned int finished[] = { 1, 1, 1 };
    evaluator_progress_t progress = {
        .generation = 0, .total = 6, .done = 3, .workers = 3, .assigned = assigned, .finished = finished,
    };
    dashboard_update_progress(dashboard, &progress); /* ignored in plain style */

    double genome[23] = { 0 };
    dashboard_generation_t first = generation_with(0, 321.5, genome);
    dashboard_add_generation(dashboard, &first);
    dashboard_generation_t twelfth = generation_with(11, 400.25, genome);
    dashboard_add_generation(dashboard, &twelfth);
    dashboard_finish(dashboard, true);

    char* text = written(stream, &buffer);
    ck_assert_ptr_null(strchr(text, '\x1b'));
    ck_assert_ptr_nonnull(strstr(text, "Training: 6 candidates x 12 generations, 2 episodes of 80 ticks each, seed 7, 3 workers."));
    ck_assert_ptr_nonnull(strstr(text, "Results: data/training_x.csv"));
    ck_assert_ptr_nonnull(strstr(text, "generation  1/12  champion   321.50"));
    ck_assert_ptr_nonnull(strstr(text, "generation 12/12  champion   400.25"));
    ck_assert_uint_eq(count_of(text, "generation "), 2);

    dashboard_destroy(&dashboard);
    fclose(stream);
    free(buffer);
}
END_TEST

START_TEST(test_dashboard_emergency_restore_shows_the_cursor_of_a_live_dashboard) {
    setenv("NO_COLOR", "1", 1);
    dashboard_emergency_restore(); /* no dashboard: nothing happens */

    FILE* file = tmpfile();
    ck_assert_ptr_nonnull(file);
    dashboard_info_t info = info_for(1, 3, "x.csv");
    dashboard_t* dashboard = NULL;
    ck_assert(dashboard_create(file, DASHBOARD_LIVE, 80, &info, &dashboard));

    dashboard_emergency_restore();
    fflush(file);
    long end = ftell(file);
    ck_assert_int_ge(end, 6);
    char tail[7] = { 0 };
    fseek(file, end - 6, SEEK_SET);
    ck_assert_uint_eq(fread(tail, 1, 6, file), 6);
    ck_assert_str_eq(tail, "\x1b[?25h");

    dashboard_destroy(&dashboard);
    fclose(file);
}
END_TEST

START_TEST(test_dashboard_detects_plain_style_when_the_output_is_not_a_terminal) {
    FILE* file = tmpfile();
    unsigned int width = 99;

    ck_assert_int_eq(dashboard_detect_style(file, 4, &width), DASHBOARD_PLAIN);
    ck_assert_uint_eq(width, 0);
    fclose(file);
}
END_TEST

Suite* dashboard_suite(void) {
    Suite* s = suite_create("dashboard");
    TCase* tc = tcase_create("core");

    tcase_add_test(tc, test_dashboard_bar_fills_in_proportion_and_a_full_bar_means_done);
    tcase_add_test(tc, test_dashboard_bar_that_does_not_fit_is_empty);
    tcase_add_test(tc, test_dashboard_sparkline_goes_from_the_lowest_to_the_highest_block);
    tcase_add_test(tc, test_dashboard_sparkline_shows_only_the_last_values_and_handles_flat_and_empty_series);
    tcase_add_test(tc, test_dashboard_levels_scale_from_zero_to_the_given_maximum);
    tcase_add_test(tc, test_dashboard_delta_shows_the_direction_and_ignores_tiny_changes);
    tcase_add_test(tc, test_dashboard_duration_uses_the_unit_that_fits);
    tcase_add_test(tc, test_dashboard_chooses_the_live_style_only_on_a_big_enough_terminal);
    tcase_add_test(tc, test_dashboard_live_panel_has_the_announced_height_and_fits_the_width);
    tcase_add_test(tc, test_dashboard_live_hides_the_cursor_redraws_in_place_and_shows_it_again);
    tcase_add_test(tc, test_dashboard_live_shows_the_fitness_its_change_and_the_history);
    tcase_add_test(tc, test_dashboard_a_run_stopped_early_does_not_show_100_percent);
    tcase_add_test(tc, test_dashboard_plain_prints_a_summary_and_one_line_per_generation_without_escapes);
    tcase_add_test(tc, test_dashboard_emergency_restore_shows_the_cursor_of_a_live_dashboard);
    tcase_add_test(tc, test_dashboard_detects_plain_style_when_the_output_is_not_a_terminal);

    suite_add_tcase(s, tc);
    return s;
}
