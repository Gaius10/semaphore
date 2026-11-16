#define _POSIX_C_SOURCE 200809L /* mkdtemp is hidden under strict -std=c2x */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/config/config.h"
#include "../src/game/game.h"
#include "../src/recording/recorder.h"
#include "suites.h"

/** The recorder on a small game in a temp folder: the header, the tick lines and the file name. */

static char dir[64];
static char config_path[128];
static char recording_path[128];
static config_t* config;
static game_t* game;
static recorder_t* recorder;

static void setup(void) {
    snprintf(dir, sizeof(dir), "/tmp/semaphore_recorder_XXXXXX");
    ck_assert_ptr_nonnull(mkdtemp(dir));
    snprintf(config_path, sizeof(config_path), "%s/config.ini", dir);
    snprintf(recording_path, sizeof(recording_path), "%s/game.jsonl", dir);
    config = NULL;
    game = NULL;
    recorder = NULL;
}

static void teardown(void) {
    recorder_close(&recorder);
    game_destroy(&game);
    config_destroy(&config);
    unlink(recording_path);
    unlink(config_path);
    rmdir(dir);
}

/** An ai-mode config whose horizontal road always spawns and vertical road never does. */
static void create_game(void) {
    FILE* f = fopen(config_path, "w");
    ck_assert_ptr_nonnull(f);
    fputs(
        "mode=ai\n"
        "agent=noop\n"
        "car_spawn_probability_h=2\n"
        "car_spawn_probability_v=0\n"
        "road_length_h=4\n"
        "road_length_v=3\n"
        "view_width=9\n"
        "view_height=7\n"
        "tick_ms=50\n",
        f
    );
    fclose(f);

    ck_assert_int_eq(config_create(config_path, &config), CONFIG_STATUS_OK);
    ck_assert_int_eq(game_create(config, &game), GAME_STATUS_OK);
}

/** Reads line `index` (0 is the header) of the recording into `line`, without its newline. */
static void read_line(unsigned int index, char* line, size_t size) {
    FILE* f = fopen(recording_path, "r");
    ck_assert_ptr_nonnull(f);
    for (unsigned int i = 0; i <= index; i++) {
        ck_assert_ptr_nonnull(fgets(line, (int) size, f));
    }
    fclose(f);
    line[strcspn(line, "\n")] = '\0';
}

START_TEST(test_recorder_writes_the_header) {
    create_game();
    ck_assert_int_eq(recorder_open(recording_path, config, &recorder), RECORDER_STATUS_OK);

    char line[512];
    read_line(0, line, sizeof(line));
    ck_assert_str_eq(line,
        "{\"format\":\"semaphore-recording\",\"version\":1,\"mode\":\"ai\",\"agent\":\"noop\","
        "\"road_length_h\":4,\"road_length_v\":3,"
        "\"car_spawn_probability_h\":2,\"car_spawn_probability_v\":0,\"tick_ms\":50}");
}
END_TEST

START_TEST(test_recorder_writes_each_tick) {
    create_game();
    ck_assert_int_eq(recorder_open(recording_path, config, &recorder), RECORDER_STATUS_OK);

    ck_assert_int_eq(recorder_write_tick(recorder, 0, game), RECORDER_STATUS_OK);
    game_command_t command = { .horizontal_light = TRAFFIC_LIGHT_GREEN, .vertical_light = TRAFFIC_LIGHT_RED };
    game_exec(game, command);
    game_exec(game, command);
    ck_assert_int_eq(recorder_write_tick(recorder, 2, game), RECORDER_STATUS_OK);

    char line[512];
    read_line(1, line, sizeof(line));
    ck_assert_str_eq(line, "{\"t\":0,\"lh\":\"g\",\"lv\":\"g\",\"h\":[],\"v\":[],\"passed\":0,\"crashes\":0}");

    /* Two ticks: car 1 spawned and moved once, car 2 just spawned. */
    read_line(2, line, sizeof(line));
    ck_assert_str_eq(line, "{\"t\":2,\"lh\":\"g\",\"lv\":\"r\",\"h\":[[1,1],[2,0]],\"v\":[],\"passed\":0,\"crashes\":0}");
}
END_TEST

START_TEST(test_recorder_never_overwrites_a_file) {
    create_game();
    FILE* f = fopen(recording_path, "w");
    ck_assert_ptr_nonnull(f);
    fclose(f);

    ck_assert_int_eq(recorder_open(recording_path, config, &recorder), RECORDER_STATUS_EEXIST);
    ck_assert_ptr_null(recorder);
}
END_TEST

START_TEST(test_recorder_default_path_has_the_time) {
    struct tm when = { .tm_year = 2026 - 1900, .tm_mon = 10, .tm_mday = 16, .tm_hour = 9, .tm_min = 5, .tm_sec = 7, .tm_isdst = -1 };
    char path[128];

    ck_assert_int_eq(recorder_default_path("recordings", mktime(&when), path, sizeof(path)), 0);
    ck_assert_str_eq(path, "recordings/game_20261116_090507.jsonl");

    ck_assert_int_eq(recorder_default_path("recordings", mktime(&when), path, 10), 1);
}
END_TEST

Suite* recorder_suite(void) {
    Suite* s = suite_create("recorder");
    TCase* tc = tcase_create("core");

    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_recorder_writes_the_header);
    tcase_add_test(tc, test_recorder_writes_each_tick);
    tcase_add_test(tc, test_recorder_never_overwrites_a_file);
    tcase_add_test(tc, test_recorder_default_path_has_the_time);

    suite_add_tcase(s, tc);
    return s;
}
