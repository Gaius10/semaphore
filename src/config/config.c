#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

struct Config {
    config_mode_t mode;
    config_human_player_t human_player;
    config_ai_player_t ai_player;
    config_view_t view;
    config_training_t training;
    unsigned int tick_ms;
    /** How many numbers the file gave for each coefficient list (the arrays are in ai_player). */
    unsigned int coeficients_h_size;
    unsigned int coeficients_v_size;
};

/**
 * Marks a field the file has not filled yet (there are no defaults). Kept out of the public
 * enum: a config returned by config_create never has it. Must stay past the last config_mode_t.
 */
#define MODE_UNSET (MODE_TRAINING + 1)
/** Same idea for the agent. Must stay past the last config_agent_t. */
#define AGENT_UNSET (CONFIG_AGENT_LINEAR_COMBINATION + 1)

static config_status_t parse_file(const char* path, config_t* config);
static config_status_t set_config_value(config_t* config, const char* key, const char* value);
static config_status_t parse_probability(const char* value, double* out);
static config_status_t parse_positive_uint(const char* value, unsigned int max, unsigned int* out);
static config_status_t parse_positive_int(const char* value, int* out);
static config_status_t parse_number_in_range(const char* value, double min, bool min_exclusive, double max, double* out);
static config_status_t parse_path(const char* value, char* out, size_t out_size);
static config_status_t parse_double_array(const char* value, double** out, unsigned int* out_size);
static bool is_complete(const config_t* config);
static bool coeficients_match_roads(const config_t* config);
static bool training_is_consistent(const config_t* config);
static void free_config(config_t* config);

config_status_t config_create(const char* path, config_t** config) {
    config_t* new_config = malloc(sizeof(config_t));
    if (new_config == NULL) {
        return CONFIG_STATUS_ENOMEM;
    }

    /** Every field starts unset, so whatever the file leaves unset is a missing key. */
    new_config->mode = MODE_UNSET;
    new_config->human_player.car_spawn_probability_h = NAN;
    new_config->human_player.car_spawn_probability_v = NAN;
    new_config->human_player.road_length_h = 0; // Valid sizes are positive, so 0 is unset
    new_config->human_player.road_length_v = 0;
    new_config->ai_player.agent = AGENT_UNSET;
    new_config->ai_player.toggle_interval_ticks = 0;
    new_config->ai_player.binarization_method = BINARIZATION_METHOD_THRESHOLD;
    new_config->ai_player.threshold = NAN; // NaN marks "unset", like the probabilities
    new_config->view.width = 0;
    new_config->view.height = 0;
    new_config->tick_ms = 0;
    new_config->ai_player.coeficients_h = NULL;
    new_config->ai_player.coeficients_v = NULL;
    new_config->coeficients_h_size = 0;
    new_config->coeficients_v_size = 0;

    /** Training: 0, NaN and the empty string mark "unset", like the sections above. */
    new_config->training = (config_training_t) {
        .crash_penalty = NAN,
        .mutation_rate = NAN,
        .mutation_sigma = NAN,
        .coeficient_max = NAN,
        .threshold_max = NAN,
    };

    config_status_t status = parse_file(path, new_config);
    if (status == CONFIG_STATUS_OK && !is_complete(new_config)) {
        status = CONFIG_STATUS_EMISSING;
    }

    /** The sizes can only be checked once the whole file is read, because key order does not matter. */
    if (status == CONFIG_STATUS_OK && !coeficients_match_roads(new_config)) {
        status = CONFIG_STATUS_EPARSE;
    }

    /** Same for the training keys that depend on each other. */
    if (status == CONFIG_STATUS_OK && !training_is_consistent(new_config)) {
        status = CONFIG_STATUS_EPARSE;
    }

    if (status != CONFIG_STATUS_OK) {
        free_config(new_config);
        return status;
    }

    *config = new_config;

    return CONFIG_STATUS_OK;
}

void config_destroy(config_t** config) {
    if (config == NULL || *config == NULL) {
        return;
    }

    free_config(*config);
    *config = NULL;
}

config_mode_t config_get_mode(const config_t* config) {
    return config->mode;
}

config_human_player_t config_get_human_player(const config_t* config) {
    return config->human_player;
}

config_ai_player_t config_get_ai_player(const config_t* config) {
    return config->ai_player;
}

config_training_t config_get_training(const config_t* config) {
    return config->training;
}

config_view_t config_get_view(const config_t* config) {
    return config->view;
}

unsigned int config_get_tick_ms(const config_t* config) {
    return config->tick_ms;
}

static config_status_t parse_file(const char* path, config_t* config) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return CONFIG_STATUS_EACCESS;
    }

    while (!feof(file)) {
        char line[MAX_LINE_LENGTH + 2]; // +2 for newline and null terminator
        if (fgets(line, sizeof(line), file) == NULL) {
            break;
        }

        if (strlen(line) > MAX_LINE_LENGTH) {
            fclose(file);
            return CONFIG_STATUS_EPARSE; // Line too long
        }

        if (line[0] == '\n') continue; // Skip empty lines
        if (line[0] == '#') continue; // Skip comments
        if (line[0] == ';') continue; // Skip comments
        if (line[0] == '[') continue; // Skip section headers

        char key[256], value[256];
        if (sscanf(line, "%255[^=]=%255[^\n]", key, value) != 2) {
            fclose(file);
            return CONFIG_STATUS_EPARSE; // Error parsing line
        }

        // Parse the line and populate the config structure
        config_status_t status = set_config_value(config, key, value);
        if (status != CONFIG_STATUS_OK) {
            // @todo Figure a better way to track this
            fclose(file);
            return status;
        }
    }

    fclose(file);
    return CONFIG_STATUS_OK;
}

static config_status_t set_config_value(config_t* config, const char* key, const char* value) {
    /** Mode */
    if (strcmp(key, "mode") == 0) {
        if (strcmp(value, "human") == 0) {
            config->mode = MODE_HUMAN_PLAYER;
        } else if (strcmp(value, "ai") == 0) {
            config->mode = MODE_AI_PLAYER;
        } else if (strcmp(value, "training") == 0) {
            config->mode = MODE_TRAINING;
        } else {
            return CONFIG_STATUS_EPARSE;
        }

        return CONFIG_STATUS_OK;
    }

    /** Human player config (read whatever the mode is, so key order in the file does not matter) */
    if (strcmp(key, "car_spawn_probability_h") == 0) {
        return parse_probability(value, &config->human_player.car_spawn_probability_h);
    }

    if (strcmp(key, "car_spawn_probability_v") == 0) {
        return parse_probability(value, &config->human_player.car_spawn_probability_v);
    }

    if (strcmp(key, "road_length_h") == 0) {
        return parse_positive_uint(value, INT_MAX, &config->human_player.road_length_h);
    }

    if (strcmp(key, "road_length_v") == 0) {
        return parse_positive_uint(value, INT_MAX, &config->human_player.road_length_v);
    }

    /** View config */
    if (strcmp(key, "view_width") == 0) {
        return parse_positive_int(value, &config->view.width);
    }

    if (strcmp(key, "view_height") == 0) {
        return parse_positive_int(value, &config->view.height);
    }

    /** Clock config */
    if (strcmp(key, "tick_ms") == 0) {
        return parse_positive_uint(value, INT_MAX, &config->tick_ms);
    }

    /** AI player config */
    if (strcmp(key, "agent") == 0) {
        if (strcmp(value, "noop") == 0) {
            config->ai_player.agent = CONFIG_AGENT_NOOP;
        } else if (strcmp(value, "fixed_toggle") == 0) {
            config->ai_player.agent = CONFIG_AGENT_FIXED_TOGGLE;
        } else if (strcmp(value, "linear_combination") == 0) {
            config->ai_player.agent = CONFIG_AGENT_LINEAR_COMBINATION;
        } else {
            return CONFIG_STATUS_EPARSE;
        }

        return CONFIG_STATUS_OK;
    }

    // Fixed toggle agent parameters
    if (strcmp(key, "toggle_interval_ticks") == 0) {
        return parse_positive_uint(value, UINT_MAX, &config->ai_player.toggle_interval_ticks);
    }

    // Linear combination agent parameters
    if (strcmp(key, "coeficients_v") == 0) {
        return parse_double_array(value, &config->ai_player.coeficients_v, &config->coeficients_v_size);
    }

    if (strcmp(key, "coeficients_h") == 0) {
        return parse_double_array(value, &config->ai_player.coeficients_h, &config->coeficients_h_size);
    }

    if (strcmp(key, "binarization_method") == 0) {
        if (strcmp(value, "threshold") == 0) {
            config->ai_player.binarization_method = BINARIZATION_METHOD_THRESHOLD;
        } else {
            return CONFIG_STATUS_EPARSE; // Unsupported binarization method
        }

        return CONFIG_STATUS_OK;
    }

    if (strcmp(key, "threshold") == 0) {
        return parse_probability(value, &config->ai_player.threshold);
    }

    /** Training config (read whatever the mode is, like the other sections) */
    if (strcmp(key, "population_size") == 0) {
        unsigned int parsed = 0;
        config_status_t status = parse_positive_uint(value, UINT_MAX, &parsed);
        if (status != CONFIG_STATUS_OK) {
            return status;
        }

        if (parsed < 2) {
            return CONFIG_STATUS_EPARSE; // Selection needs at least two candidates
        }

        config->training.population_size = parsed;
        return CONFIG_STATUS_OK;
    }

    if (strcmp(key, "generations") == 0) {
        return parse_positive_uint(value, UINT_MAX, &config->training.generations);
    }

    if (strcmp(key, "episodes_per_candidate") == 0) {
        return parse_positive_uint(value, UINT_MAX, &config->training.episodes_per_candidate);
    }

    if (strcmp(key, "episode_ticks") == 0) {
        return parse_positive_uint(value, UINT_MAX, &config->training.episode_ticks);
    }

    if (strcmp(key, "crash_penalty") == 0) {
        return parse_number_in_range(value, 0.0, false, INFINITY, &config->training.crash_penalty);
    }

    if (strcmp(key, "elite_count") == 0) {
        return parse_positive_uint(value, UINT_MAX, &config->training.elite_count);
    }

    if (strcmp(key, "tournament_size") == 0) {
        return parse_positive_uint(value, UINT_MAX, &config->training.tournament_size);
    }

    if (strcmp(key, "mutation_rate") == 0) {
        return parse_number_in_range(value, 0.0, false, 1.0, &config->training.mutation_rate);
    }

    if (strcmp(key, "mutation_sigma") == 0) {
        return parse_number_in_range(value, 0.0, true, INFINITY, &config->training.mutation_sigma);
    }

    if (strcmp(key, "coeficient_max") == 0) {
        return parse_number_in_range(value, 0.0, true, INFINITY, &config->training.coeficient_max);
    }

    if (strcmp(key, "threshold_max") == 0) {
        return parse_number_in_range(value, 0.0, true, INFINITY, &config->training.threshold_max);
    }

    if (strcmp(key, "seed") == 0) {
        return parse_positive_uint(value, UINT_MAX, &config->training.seed);
    }

    if (strcmp(key, "workers") == 0) {
        return parse_positive_uint(value, UINT_MAX, &config->training.workers);
    }

    if (strcmp(key, "training_output_dir") == 0) {
        return parse_path(value, config->training.output_dir, sizeof(config->training.output_dir));
    }

    // Add more key-value pairs as needed for other configuration options.
    // Unknown keys are ignored.
    return CONFIG_STATUS_OK;
}

/** Accepts a whole number, optionally followed by whitespace (e.g. a trailing '\r'). Rejects NaN, which marks "unset". */
static config_status_t parse_probability(const char* value, double* out) {
    char* end = NULL;
    double parsed = strtod(value, &end);
    if (end == value || isnan(parsed)) {
        return CONFIG_STATUS_EPARSE;
    }

    while (isspace((unsigned char) *end)) {
        end++;
    }

    if (*end != '\0') {
        return CONFIG_STATUS_EPARSE;
    }

    *out = parsed;
    return CONFIG_STATUS_OK;
}

/**
 * Accepts a finite number from `min` to `max` (`min` itself only when `min_exclusive` is false),
 * optionally followed by whitespace. *out is only written on success.
 */
static config_status_t parse_number_in_range(const char* value, double min, bool min_exclusive, double max, double* out) {
    double parsed = 0.0;
    config_status_t status = parse_probability(value, &parsed);
    if (status != CONFIG_STATUS_OK) {
        return status;
    }

    if (!isfinite(parsed) || parsed < min || (min_exclusive && parsed == min) || parsed > max) {
        return CONFIG_STATUS_EPARSE;
    }

    *out = parsed;
    return CONFIG_STATUS_OK;
}

/**
 * Accepts a whole number from 1 to `max`, optionally followed by whitespace. Zero marks "unset".
 * Parsed as signed on purpose: strtoul would take "-3" and wrap it to a huge value.
 */
static config_status_t parse_positive_uint(const char* value, unsigned int max, unsigned int* out) {
    char* end = NULL;
    errno = 0;
    long long parsed = strtoll(value, &end, 10);
    if (end == value || errno == ERANGE || parsed < 1 || (unsigned long long) parsed > max) {
        return CONFIG_STATUS_EPARSE;
    }

    while (isspace((unsigned char) *end)) {
        end++;
    }

    if (*end != '\0') {
        return CONFIG_STATUS_EPARSE;
    }

    *out = (unsigned int) parsed;
    return CONFIG_STATUS_OK;
}

/** Same as parse_positive_uint, into an int (so up to INT_MAX). */
static config_status_t parse_positive_int(const char* value, int* out) {
    unsigned int parsed = 0;
    config_status_t status = parse_positive_uint(value, INT_MAX, &parsed);
    if (status != CONFIG_STATUS_OK) {
        return status;
    }

    *out = (int) parsed;
    return CONFIG_STATUS_OK;
}

/** True when every field the config's mode needs has been filled. */
static bool is_complete(const config_t* config) {
    if (config->mode == MODE_UNSET) {
        return false;
    }

    /** The AI plays the same world as the human, so both modes need the same keys. */
    if (config->mode == MODE_AI_PLAYER) {
        if (config->ai_player.agent == AGENT_UNSET) {
            return false;
        }

        if (config->ai_player.agent == CONFIG_AGENT_FIXED_TOGGLE && config->ai_player.toggle_interval_ticks == 0) {
            return false;
        }

        if (config->ai_player.agent == CONFIG_AGENT_LINEAR_COMBINATION) {
            if (config->ai_player.coeficients_h == NULL || config->ai_player.coeficients_v == NULL) {
                return false;
            }

            /** The threshold only matters (and is only required) for the threshold binarization. */
            if (config->ai_player.binarization_method == BINARIZATION_METHOD_THRESHOLD
                    && isnan(config->ai_player.threshold)) {
                return false;
            }
        }
    }

    bool world_is_set = !isnan(config->human_player.car_spawn_probability_h)
        && !isnan(config->human_player.car_spawn_probability_v)
        && config->human_player.road_length_h > 0
        && config->human_player.road_length_v > 0;

    if (config->mode == MODE_HUMAN_PLAYER || config->mode == MODE_AI_PLAYER) {
        return world_is_set
            && config->view.width > 0
            && config->view.height > 0
            && config->tick_ms > 0;
    }

    /** Training plays the same world but has no view and no clock. */
    const config_training_t* training = &config->training;
    return world_is_set
        && training->population_size > 0
        && training->generations > 0
        && training->episodes_per_candidate > 0
        && training->episode_ticks > 0
        && !isnan(training->crash_penalty)
        && training->elite_count > 0
        && training->tournament_size > 0
        && !isnan(training->mutation_rate)
        && !isnan(training->mutation_sigma)
        && !isnan(training->coeficient_max)
        && !isnan(training->threshold_max)
        && training->seed > 0
        && training->workers > 0
        && training->output_dir[0] != '\0';
}

/** True unless training is selected and its keys do not fit together (they only make sense once the whole file is read). */
static bool training_is_consistent(const config_t* config) {
    if (config->mode != MODE_TRAINING) {
        return true;
    }

    return config->training.elite_count < config->training.population_size
        && config->training.tournament_size <= config->training.population_size;
}

/**
 * True unless the linear combination agent is selected and its coefficient lists do not have one
 * number per cell of their road. Only that agent cares, so the lists are ignored for the others.
 */
static bool coeficients_match_roads(const config_t* config) {
    if (config->mode != MODE_AI_PLAYER || config->ai_player.agent != CONFIG_AGENT_LINEAR_COMBINATION) {
        return true;
    }

    return config->coeficients_h_size == config->human_player.road_length_h
        && config->coeficients_v_size == config->human_player.road_length_v;
}

/**
 * Parses a comma-separated list of numbers, like "0.5, 0.25, 1", into a new array that replaces
 * *out (the old one is freed). Whitespace around each number is fine. An empty list, an empty
 * entry (e.g. a trailing comma) or a value that is not a finite number is an error, and then
 * *out and *out_size are left untouched.
 */
static config_status_t parse_double_array(const char* value, double** out, unsigned int* out_size) {
    unsigned int count = 1;
    for (const char* c = value; *c != '\0'; c++) {
        if (*c == ',') {
            count++;
        }
    }

    double* parsed = malloc(count * sizeof(double));
    if (parsed == NULL) {
        return CONFIG_STATUS_ENOMEM;
    }

    const char* cursor = value;
    for (unsigned int i = 0; i < count; i++) {
        char* end = NULL;
        double number = strtod(cursor, &end);
        if (end == cursor || !isfinite(number)) {
            free(parsed);
            return CONFIG_STATUS_EPARSE;
        }

        while (isspace((unsigned char) *end)) {
            end++;
        }

        /** A number is followed by a comma, except the last one, which ends the value. */
        char expected = i + 1 < count ? ',' : '\0';
        if (*end != expected) {
            free(parsed);
            return CONFIG_STATUS_EPARSE;
        }

        parsed[i] = number;
        cursor = end + 1;
    }

    free(*out);
    *out = parsed;
    *out_size = count;

    return CONFIG_STATUS_OK;
}

static void free_config(config_t* config) {
    free(config->ai_player.coeficients_h);
    free(config->ai_player.coeficients_v);
    free(config);
}

/** Copies a path without the whitespace around it. Rejects an empty path and one that does not fit in `out`. */
static config_status_t parse_path(const char* value, char* out, size_t out_size) {
    while (isspace((unsigned char) *value)) {
        value++;
    }

    size_t length = strlen(value);
    while (length > 0 && isspace((unsigned char) value[length - 1])) {
        length--;
    }

    if (length == 0 || length >= out_size) {
        return CONFIG_STATUS_EPARSE;
    }

    memcpy(out, value, length);
    out[length] = '\0';

    return CONFIG_STATUS_OK;
}
