#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

static void set_config_value(config_t* config, const char* key, const char* value);

config_status_t config_parse_file(const char* path, config_t* config) {
    FILE* file = fopen(path, "r");

    if (file == NULL) {
        return CONFIG_STATUS_EACCESS;
    }

    while (!feof(file)) {
        char line[MAX_LINE_LENGTH + 2]; // +2 for newline and null terminator
        if (fgets(line, sizeof(line), file) == NULL) {
            break;
        }

        if (line[0] == '\n') continue; // Skip empty lines
        if (line[0] == '#') continue; // Skip comments
        if (line[0] == ';') continue; // Skip comments
        if (line[0] == '[') continue; // Skip section headers

        char key[256], value[256];
        if (sscanf(line, "%255[^=]=%255[^\n]", key, value) != 2) {
            return CONFIG_STATUS_EPARSE; // Error parsing line
        }

        // Parse the line and populate the config structure
        set_config_value(config, key, value);
    }

    fclose(file);
    return CONFIG_STATUS_OK;
}

static void set_config_value(config_t* config, const char* key, const char* value) {
    /** Read configured mode */
    if (strcmp(key, "mode") == 0) {
        if (strcmp(value, "human") == 0) {
            config->mode = MODE_HUMAN_PLAYER;
        } else if (strcmp(value, "ai") == 0) {
            config->mode = MODE_AI_PLAYER;
        } else if (strcmp(value, "training") == 0) {
            config->mode = MODE_TRAINING;
        }
    }

    /** Human player config */
    if (config->mode == MODE_HUMAN_PLAYER) {
        // Parse human player specific configuration options
        if (strcmp(key, "car_spawn_probability_h") == 0) {
            sscanf(value, "%lf", &config->human_player->car_spawn_probability_h);
        } else if (strcmp(key, "car_spawn_probability_v") == 0) {
            sscanf(value, "%lf", &config->human_player->car_spawn_probability_v);
        }
    }

    /** AI player config */
    if (config->mode == MODE_AI_PLAYER) {
        // Parse AI player specific configuration options
    }

    /** Training config */
    if (config->mode == MODE_TRAINING) {
        // Parse training specific configuration options
    }

    // Add more key-value pairs as needed for other configuration options.
}
