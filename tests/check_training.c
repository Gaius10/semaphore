#define _POSIX_C_SOURCE 200809L /* mkdtemp and opendir are hidden under strict -std=c2x */

#include <check.h>
#include <dirent.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../src/config/config.h"
#include "../src/mode/training.h"
#include "../src/training/candidate.h"
#include "../src/training/results_csv.h"
#include "suites.h"

/**
 * Training mode end to end, on a tiny run in a temp folder: the CSV it writes, what it prints, and
 * the results file module on its own.
 */

#define WORLD_H 10
#define WORLD_V 12
#define COLUMNS (7 + WORLD_H + WORLD_V)
#define GENERATIONS 4
#define POPULATION 6
#define COEFICIENT_MAX 2.0
#define THRESHOLD_MAX 4.0

static char dir[64];
static char config_path[128];
static char output_path[128]; /* where stdout goes while the mode runs */
static config_t* config;

static void setup(void) {
    snprintf(dir, sizeof(dir), "/tmp/semaphore_training_XXXXXX");
    ck_assert_ptr_nonnull(mkdtemp(dir));
    snprintf(config_path, sizeof(config_path), "%s/config.ini", dir);
    snprintf(output_path, sizeof(output_path), "%s/stdout.txt", dir);
    config = NULL;
}

/** Removes every file of the temp folder (and the folders `training_output_dir` makes in it), then the folder. */
static void remove_tree(const char* path) {
    DIR* d = opendir(path);
    if (d != NULL) {
        struct dirent* entry;
        while ((entry = readdir(d)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char child[512];
            snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
            struct stat info;
            if (lstat(child, &info) == 0 && S_ISDIR(info.st_mode)) {
                remove_tree(child);
            } else {
                unlink(child);
            }
        }
        closedir(d);
    }
    rmdir(path);
}

static void teardown(void) {
    config_destroy(&config);
    remove_tree(dir);
}

/** Writes a training config whose results go to `output_dir`, and builds `config` from it. */
static void make_config(const char* output_dir, unsigned int seed) {
    FILE* f = fopen(config_path, "w");
    ck_assert_ptr_nonnull(f);
    fprintf(
        f,
        "mode=training\n"
        "car_spawn_probability_h=0.5\n"
        "car_spawn_probability_v=0.5\n"
        "road_length_h=%d\n"
        "road_length_v=%d\n"
        "population_size=%d\n"
        "generations=%d\n"
        "episodes_per_candidate=2\n"
        "episode_ticks=80\n"
        "crash_penalty=5\n"
        "elite_count=1\n"
        "tournament_size=2\n"
        "mutation_rate=0.2\n"
        "mutation_sigma=0.2\n"
        "coeficient_max=%g\n"
        "threshold_max=%g\n"
        "seed=%u\n"
        "workers=1\n"
        "training_output_dir=%s\n",
        WORLD_H, WORLD_V, POPULATION, GENERATIONS, COEFICIENT_MAX, THRESHOLD_MAX, seed, output_dir
    );
    fclose(f);

    config_destroy(&config);
    ck_assert_int_eq(config_create(config_path, &config), CONFIG_STATUS_OK);
}

/** Runs the mode with stdout redirected to a file (each test runs in its own process, so it is not restored). */
static int run_mode(void) {
    ck_assert_ptr_nonnull(freopen(output_path, "w", stdout));
    int code = mode_run_training(config);
    fflush(stdout);
    return code;
}

/** The whole file as a string. The caller frees it. */
static char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    ck_assert_ptr_nonnull(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* text = malloc((size_t)size + 1);
    ck_assert_ptr_nonnull(text);
    ck_assert_uint_eq(fread(text, 1, (size_t)size, f), (size_t)size);
    text[size] = '\0';
    fclose(f);
    return text;
}

/** Finds the one training_*.csv in `folder` and writes its path. */
static void find_csv(const char* folder, char* path, size_t size) {
    DIR* d = opendir(folder);
    ck_assert_ptr_nonnull(d);

    int found = 0;
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, "training_", 9) == 0 && strstr(entry->d_name, ".csv") != NULL) {
            snprintf(path, size, "%s/%s", folder, entry->d_name);
            found++;
        }
    }
    closedir(d);
    ck_assert_int_eq(found, 1);
}

/** Splits one line into numbers. Returns how many cells it had; fails the test if a cell is not a number. */
static int parse_row(char* line, double* cells, int max_cells) {
    int count = 0;
    char* rest = NULL; /* strtok_r: the callers are in the middle of their own strtok over the lines */
    for (char* cell = strtok_r(line, ",\n", &rest); cell != NULL; cell = strtok_r(NULL, ",\n", &rest)) {
        char* end = NULL;
        ck_assert_int_lt(count, max_cells);
        cells[count] = strtod(cell, &end);
        ck_assert_msg(end != cell && *end == '\0', "cell '%s' is not a number", cell);
        count++;
    }
    return count;
}

/* ------------------------------------------------------------ the mode */

START_TEST(test_training_writes_a_header_and_one_row_per_generation) {
    char folder[128];
    snprintf(folder, sizeof(folder), "%s/out", dir);
    make_config(folder, 5);

    ck_assert_int_eq(run_mode(), 0);

    char csv[256];
    find_csv(folder, csv, sizeof(csv));
    char* text = read_file(csv);

    char* line = strtok(text, "\n");
    ck_assert_ptr_nonnull(line);
    ck_assert(strncmp(line, "generation,champion_fitness,mean_fitness,worst_fitness,", 55) == 0);
    ck_assert_ptr_nonnull(strstr(line, ",champion_cars_passed,champion_crashes,threshold,h0,h1,"));
    ck_assert_ptr_nonnull(strstr(line, ",h9,v0,v1,"));
    ck_assert_ptr_nonnull(strstr(line, ",v11")); /* the last column */
    ck_assert_ptr_null(strstr(line, "h10"));
    ck_assert_ptr_null(strstr(line, "v12"));

    int rows = 0;
    while ((line = strtok(NULL, "\n")) != NULL) {
        double cells[COLUMNS];
        ck_assert_int_eq(parse_row(line, cells, COLUMNS), COLUMNS);
        ck_assert_double_eq(cells[0], rows); /* generation */
        rows++;
    }
    ck_assert_int_eq(rows, GENERATIONS);

    free(text);
}
END_TEST

START_TEST(test_training_rows_are_consistent_with_each_other) {
    make_config(dir, 6);
    ck_assert_int_eq(run_mode(), 0);

    char csv[256];
    find_csv(dir, csv, sizeof(csv));
    char* text = read_file(csv);

    char* line = strtok(text, "\n"); /* header */
    int rows = 0;
    while ((line = strtok(NULL, "\n")) != NULL) {
        double c[COLUMNS];
        ck_assert_int_eq(parse_row(line, c, COLUMNS), COLUMNS);

        double champion = c[1], mean = c[2], worst = c[3], cars = c[4], crashes = c[5];
        ck_assert_double_ge(champion, mean);
        ck_assert_double_ge(mean, worst);
        ck_assert_double_eq_tol(champion, cars - 5.0 * crashes, 1e-9); /* crash_penalty=5 */
        ck_assert_double_gt(cars, 0);

        ck_assert_double_ge(c[6], 0.0); /* threshold */
        ck_assert_double_le(c[6], THRESHOLD_MAX);
        for (int g = 7; g < COLUMNS; g++) {
            ck_assert_double_ge(c[g], 0.0);
            ck_assert_double_le(c[g], COEFICIENT_MAX);
        }
        rows++;
    }
    ck_assert_int_eq(rows, GENERATIONS);

    free(text);
}
END_TEST

START_TEST(test_training_every_row_can_be_rescored_from_its_own_genome) {
    /* The CSV keeps the champion in full: playing its genome again gives the fitness on the same row. */
    make_config(dir, 12);
    ck_assert_int_eq(run_mode(), 0);

    char csv[256];
    find_csv(dir, csv, sizeof(csv));
    char* text = read_file(csv);

    char* line = strtok(text, "\n"); /* header */
    int rows = 0;
    while ((line = strtok(NULL, "\n")) != NULL) {
        double c[COLUMNS];
        ck_assert_int_eq(parse_row(line, c, COLUMNS), COLUMNS);

        candidate_result_t again;
        ck_assert_int_eq(candidate_play(config, &c[6], (unsigned int)c[0], &again), CANDIDATE_STATUS_OK);
        ck_assert_double_eq_tol(again.fitness, c[1], 1e-9);
        ck_assert_double_eq_tol(again.cars_passed, c[4], 1e-9);
        rows++;
    }
    ck_assert_int_eq(rows, GENERATIONS);

    free(text);
}
END_TEST

START_TEST(test_training_same_seed_gives_the_same_results_and_a_different_seed_does_not) {
    char folders[3][128];
    char* texts[3];
    unsigned int seeds[3] = { 9, 9, 10 };

    for (int run = 0; run < 3; run++) {
        snprintf(folders[run], sizeof(folders[run]), "%s/run%d", dir, run);
        make_config(folders[run], seeds[run]);
        /* One process per run: stdout is redirected and the output of each must go to its own file. */
        pid_t pid = fork();
        ck_assert_int_ge(pid, 0);
        if (pid == 0) {
            _exit(run_mode());
        }
        int status = 0;
        ck_assert_int_eq(waitpid(pid, &status, 0), pid);
        ck_assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);

        char csv[256];
        find_csv(folders[run], csv, sizeof(csv));
        texts[run] = read_file(csv);
    }

    ck_assert_str_eq(texts[0], texts[1]);
    ck_assert_str_ne(texts[0], texts[2]);

    for (int run = 0; run < 3; run++) {
        free(texts[run]);
    }
}
END_TEST

START_TEST(test_training_prints_progress_and_the_best_genome_to_paste) {
    make_config(dir, 7);
    ck_assert_int_eq(run_mode(), 0);

    char* out = read_file(output_path);
    ck_assert_ptr_nonnull(strstr(out, "generation 1/4"));
    ck_assert_ptr_nonnull(strstr(out, "generation 4/4"));
    ck_assert_ptr_nonnull(strstr(out, "agent=linear_combination"));
    ck_assert_ptr_nonnull(strstr(out, "threshold="));
    ck_assert_ptr_nonnull(strstr(out, "coeficients_h="));
    ck_assert_ptr_nonnull(strstr(out, "coeficients_v="));
    ck_assert_ptr_nonnull(strstr(out, ".csv"));

    /* The coefficient lines have exactly one number per road cell. */
    const char* h = strstr(out, "coeficients_h=");
    const char* v = strstr(out, "coeficients_v=");
    int commas_h = 0, commas_v = 0;
    for (const char* p = h; *p != '\n'; p++) {
        commas_h += *p == ',';
    }
    for (const char* p = v; *p != '\n'; p++) {
        commas_v += *p == ',';
    }
    ck_assert_int_eq(commas_h, WORLD_H - 1);
    ck_assert_int_eq(commas_v, WORLD_V - 1);

    free(out);
}
END_TEST

START_TEST(test_training_creates_the_output_folder) {
    char folder[128];
    snprintf(folder, sizeof(folder), "%s/not_there_yet", dir);
    make_config(folder, 8);

    ck_assert_int_eq(run_mode(), 0);

    struct stat info;
    ck_assert_int_eq(stat(folder, &info), 0);
    ck_assert(S_ISDIR(info.st_mode));
}
END_TEST

START_TEST(test_training_fails_when_the_output_folder_cannot_be_made) {
    char folder[160];
    snprintf(folder, sizeof(folder), "%s/missing_parent/out", dir); /* only one level is created */
    make_config(folder, 9);

    ck_assert_int_ne(run_mode(), 0);

    char* out = read_file(output_path);
    ck_assert_ptr_nonnull(strstr(out, "Error"));
    free(out);
}
END_TEST

/* ------------------------------------------------------- the results file */

static results_csv_t* open_csv(const char* path, unsigned int h, unsigned int v) {
    results_csv_t* csv = NULL;
    ck_assert_int_eq(results_csv_open(path, h, v, &csv), RESULTS_CSV_STATUS_OK);
    return csv;
}

START_TEST(test_results_csv_default_path_has_the_folder_and_a_timestamp) {
    struct tm local = { .tm_year = 2026 - 1900, .tm_mon = 10, .tm_mday = 16, .tm_hour = 9, .tm_min = 5, .tm_sec = 3, .tm_isdst = -1 };
    time_t when = mktime(&local);
    char path[128];

    ck_assert_int_eq(results_csv_default_path("data", when, path, sizeof(path)), 0);
    ck_assert_str_eq(path, "data/training_20261116_090503.csv");

    ck_assert_int_ne(results_csv_default_path("data", when, path, 10), 0); /* does not fit */
}
END_TEST

START_TEST(test_results_csv_rows_read_back_exactly) {
    char path[128];
    snprintf(path, sizeof(path), "%s/exact.csv", dir);
    results_csv_t* csv = open_csv(path, 2, 1);

    double genome[4] = { 0.1, 1.0 / 3.0, 2.5e-9, 123456789.123456789 };
    results_row_t row = {
        .generation = 7,
        .champion_fitness = 1.0 / 7.0,
        .mean_fitness = 90.5,
        .worst_fitness = -3.25,
        .champion_cars_passed = 468.3333333333333,
        .champion_crashes = 0.0,
        .genome = genome,
    };
    ck_assert_int_eq(results_csv_write(csv, &row), RESULTS_CSV_STATUS_OK);

    /* Flushed already: readable before the file is closed, as after an interrupted run. */
    char* text = read_file(path);
    char* header = strtok(text, "\n");
    ck_assert_str_eq(
        header,
        "generation,champion_fitness,mean_fitness,worst_fitness,champion_cars_passed,champion_crashes,threshold,h0,h1,v0"
    );

    double cells[10];
    ck_assert_int_eq(parse_row(strtok(NULL, "\n"), cells, 10), 10);
    ck_assert_double_eq(cells[0], 7);
    ck_assert_double_eq(cells[1], row.champion_fitness);
    ck_assert_double_eq(cells[2], row.mean_fitness);
    ck_assert_double_eq(cells[3], row.worst_fitness);
    ck_assert_double_eq(cells[4], row.champion_cars_passed);
    ck_assert_double_eq(cells[5], row.champion_crashes);
    for (int g = 0; g < 4; g++) {
        ck_assert_double_eq(cells[6 + g], genome[g]);
    }
    free(text);

    ck_assert_int_eq(results_csv_close(&csv), RESULTS_CSV_STATUS_OK);
    ck_assert_ptr_null(csv);
}
END_TEST

START_TEST(test_results_csv_never_overwrites_an_existing_file) {
    char path[128];
    snprintf(path, sizeof(path), "%s/keep.csv", dir);

    FILE* f = fopen(path, "w");
    ck_assert_ptr_nonnull(f);
    fputs("precious\n", f);
    fclose(f);

    results_csv_t* csv = NULL;
    ck_assert_int_eq(results_csv_open(path, 2, 1, &csv), RESULTS_CSV_STATUS_EEXIST);
    ck_assert_ptr_null(csv);

    char* text = read_file(path);
    ck_assert_str_eq(text, "precious\n");
    free(text);
}
END_TEST

START_TEST(test_results_csv_open_fails_in_a_missing_folder_and_close_is_safe_on_null) {
    results_csv_t* csv = NULL;
    char path[160];
    snprintf(path, sizeof(path), "%s/nope/file.csv", dir);

    ck_assert_int_eq(results_csv_open(path, 2, 1, &csv), RESULTS_CSV_STATUS_EIO);
    ck_assert_ptr_null(csv);
    ck_assert_int_eq(results_csv_close(&csv), RESULTS_CSV_STATUS_OK);
    ck_assert_int_eq(results_csv_close(NULL), RESULTS_CSV_STATUS_OK);
}
END_TEST

Suite* training_suite(void) {
    Suite* s = suite_create("training");
    TCase* tc = tcase_create("core");
    tcase_set_timeout(tc, 60);
    tcase_add_checked_fixture(tc, setup, teardown);

    tcase_add_test(tc, test_training_writes_a_header_and_one_row_per_generation);
    tcase_add_test(tc, test_training_rows_are_consistent_with_each_other);
    tcase_add_test(tc, test_training_every_row_can_be_rescored_from_its_own_genome);
    tcase_add_test(tc, test_training_same_seed_gives_the_same_results_and_a_different_seed_does_not);
    tcase_add_test(tc, test_training_prints_progress_and_the_best_genome_to_paste);
    tcase_add_test(tc, test_training_creates_the_output_folder);
    tcase_add_test(tc, test_training_fails_when_the_output_folder_cannot_be_made);
    tcase_add_test(tc, test_results_csv_default_path_has_the_folder_and_a_timestamp);
    tcase_add_test(tc, test_results_csv_rows_read_back_exactly);
    tcase_add_test(tc, test_results_csv_never_overwrites_an_existing_file);
    tcase_add_test(tc, test_results_csv_open_fails_in_a_missing_folder_and_close_is_safe_on_null);

    suite_add_tcase(s, tc);
    return s;
}
