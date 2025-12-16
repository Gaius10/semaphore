#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "game/game.h"

unsigned int parse_args(int argc, char** argv, config_t* config);
void print_usage();

int main(int argc, char** argv) {
    config_t config;

    if (parse_args(argc, argv, &config) != 0) {
        print_usage();
        return 1;
    }

    if (config.mode == MODE_HELP) {
        print_usage();
        return 0;
    }

    if (config.mode == MODE_DEFAULT) {
        return game_play(config) == GAME_OK ? 0 : 1;
    }

    printf("Mode not yet implemented.\n");
    return 1;
}

void print_usage() {
    printf("Usage: semaphore [-h [--config <path>]]\n");
}

unsigned int parse_args(int argc, char** argv, config_t* config) {
    config->mode = MODE_DEFAULT;

    if (argc > 3) {
        config->mode = MODE_HELP;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            config->mode = MODE_HELP;
            continue;
        }

        if (strcmp(argv[i], "--config") == 0) {
            if (config_parse_file(argv[++i], config) != CONFIG_OK) {
                printf("Error parsing config file.\n");
                return 1;
            }

            continue;
        }

        printf("Error: invalid flag '%s'\n", argv[i]);
        return 1;
    }

    return 0;
}
