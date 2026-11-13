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
    unsigned int tick_ms;
};

/**
 * Marks a field the file has not filled yet (there are no defaults). Kept out of the public
 * enum: a config returned by config_create never has it. Must stay past the last config_mode_t.
 */
#define MODE_UNSET (MODE_TRAINING + 1)
/** Same idea for the agent. Must stay past the last config_agent_t. */
#define AGENT_UNSET (CONFIG_AGENT_FIXED_TOGGLE + 1)

static config_status_t parse_file(const char* path, config_t* config);
static config_status_t set_config_value(config_t* config, const char* key, const char* value);
static config_status_t parse_probability(const char* value, double* out);
static config_status_t parse_positive_uint(const char* value, unsigned int max, unsigned int* out);
static config_status_t parse_positive_int(const char* value, int* out);
static bool is_complete(const config_t* config);

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
    new_config->view.width = 0;
    new_config->view.height = 0;
    new_config->tick_ms = 0;

    config_status_t status = parse_file(path, new_config);
    if (status == CONFIG_STATUS_OK && !is_complete(new_config)) {
        status = CONFIG_STATUS_EMISSING;
    }

    if (status != CONFIG_STATUS_OK) {
        free(new_config);
        return status;
    }

    *config = new_config;

    return CONFIG_STATUS_OK;
}

void config_destroy(config_t** config) {
    if (config == NULL || *config == NULL) {
        return;
    }

    free(*config);
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
        } else {
            return CONFIG_STATUS_EPARSE;
        }

        return CONFIG_STATUS_OK;
    }

    if (strcmp(key, "toggle_interval_ticks") == 0) {
        return parse_positive_uint(value, INT_MAX, &config->ai_player.toggle_interval_ticks);
    }

    /** Training config */
    // Parse training specific configuration options

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
    }

    if (config->mode == MODE_HUMAN_PLAYER || config->mode == MODE_AI_PLAYER) {
        return !isnan(config->human_player.car_spawn_probability_h)
            && !isnan(config->human_player.car_spawn_probability_v)
            && config->human_player.road_length_h > 0
            && config->human_player.road_length_v > 0
            && config->view.width > 0
            && config->view.height > 0
            && config->tick_ms > 0;
    }

    return true;
}
