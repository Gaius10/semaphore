#define _POSIX_C_SOURCE 200809L /* localtime_r and mkdir are hidden under strict -std=c2x */

#include "recorder.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../game/entities/car.h"
#include "../game/entities/road.h"
#include "../game/entities/traffic_light.h"

struct recorder {
    FILE* file;
};

static const char* agent_name(const config_t* config);
static void write_road(FILE* file, const road_t* road, int horizontal);
static char light_char(const traffic_light_t* light);

int recorder_default_path(const char* dir, time_t when, char* path, size_t size) {
    struct tm local;
    char stamp[32];

    if (localtime_r(&when, &local) == NULL || strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &local) == 0) {
        return 1;
    }

    int written = snprintf(path, size, "%s/game_%s.jsonl", dir, stamp);
    return written < 0 || (size_t)written >= size;
}

recorder_status_t recorder_open(const char* path, const config_t* config, recorder_t** recorder) {
    recorder_t* self = malloc(sizeof(recorder_t));
    if (self == NULL) {
        return RECORDER_STATUS_ENOMEM;
    }

    self->file = fopen(path, "wx"); // "x": fail if the file exists, so no recording is overwritten
    if (self->file == NULL) {
        recorder_status_t status = errno == EEXIST ? RECORDER_STATUS_EEXIST : RECORDER_STATUS_EIO;
        free(self);
        return status;
    }

    config_human_player_t world = config_get_human_player(config);
    const char* mode = config_get_mode(config) == MODE_AI_PLAYER ? "ai" : "human";
    const char* agent = agent_name(config);

    fprintf(self->file, "{\"format\":\"semaphore-recording\",\"version\":1,\"mode\":\"%s\",", mode);
    if (agent != NULL) {
        fprintf(self->file, "\"agent\":\"%s\",", agent);
    } else {
        fputs("\"agent\":null,", self->file);
    }
    fprintf(
        self->file,
        "\"road_length_h\":%u,\"road_length_v\":%u,"
        "\"car_spawn_probability_h\":%.17g,\"car_spawn_probability_v\":%.17g,\"tick_ms\":%u}\n",
        world.road_length_h,
        world.road_length_v,
        world.car_spawn_probability_h,
        world.car_spawn_probability_v,
        config_get_tick_ms(config)
    );

    if (fflush(self->file) != 0) {
        fclose(self->file);
        free(self);
        return RECORDER_STATUS_EIO;
    }

    *recorder = self;
    return RECORDER_STATUS_OK;
}

recorder_status_t recorder_open_in(const char* dir, const config_t* config, char* path, size_t size, recorder_t** recorder) {
    if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
        return RECORDER_STATUS_EIO;
    }

    if (recorder_default_path(dir, time(NULL), path, size) != 0) {
        return RECORDER_STATUS_EIO;
    }

    return recorder_open(path, config, recorder);
}

recorder_status_t recorder_write_tick(recorder_t* recorder, unsigned int tick, const game_t* game) {
    FILE* file = recorder->file;

    fprintf(
        file,
        "{\"t\":%u,\"lh\":\"%c\",\"lv\":\"%c\",\"h\":",
        tick,
        light_char(game_get_horizontal_light(game)),
        light_char(game_get_vertical_light(game))
    );
    write_road(file, game_get_horizontal_road(game), 1);
    fputs(",\"v\":", file);
    write_road(file, game_get_vertical_road(game), 0);
    fprintf(file, ",\"passed\":%u,\"crashes\":%u}\n", game_get_cars_passed(game), game_get_crashes(game));

    if (ferror(file) || fflush(file) != 0) {
        return RECORDER_STATUS_EIO;
    }

    return RECORDER_STATUS_OK;
}

void recorder_close(recorder_t** recorder) {
    if (recorder == NULL || *recorder == NULL) {
        return;
    }

    fclose((*recorder)->file);
    free(*recorder);
    *recorder = NULL;
}

static const char* agent_name(const config_t* config) {
    if (config_get_mode(config) != MODE_AI_PLAYER) {
        return NULL;
    }

    switch (config_get_ai_player(config).agent) {
        case CONFIG_AGENT_NOOP:
            return "noop";
        case CONFIG_AGENT_FIXED_TOGGLE:
            return "fixed_toggle";
        case CONFIG_AGENT_LINEAR_COMBINATION:
            return "linear_combination";
    }

    return NULL;
}

/** [[id, position], ...]: position is x on the horizontal road and y on the vertical one. */
static void write_road(FILE* file, const road_t* road, int horizontal) {
    const char* separator = "";

    fputc('[', file);
    for (int i = 0; i < road_size(road); i++) {
        car_t* car = NULL;
        if (road_get(road, i, &car) != ROAD_STATUS_OK) {
            continue;
        }

        int position = horizontal ? car_get_x(car) : car_get_y(car);
        fprintf(file, "%s[%u,%d]", separator, car_get_id(car), position);
        separator = ",";
    }
    fputc(']', file);
}

static char light_char(const traffic_light_t* light) {
    return traffic_light_get_color(light) == TRAFFIC_LIGHT_GREEN ? 'g' : 'r';
}
