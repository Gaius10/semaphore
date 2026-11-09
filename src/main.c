#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "config/config.h"
#include "game/game.h"

static int mode_run_human_player(const config_t* config);

int main(int argc, char** argv) {
    srand((unsigned int) time(NULL));

    config_t config;
    config_status_t _config = config_parse_file("config.ini", &config);
    if (_config == CONFIG_STATUS_EACCESS) {
        printf("Error accessing config file.\n");
        printf("Make sure the file 'config.ini' exists and is readable.\n");
        return 1;
    }

    if (_config == CONFIG_STATUS_EPARSE) {
        printf("Error parsing config file.\n");
        printf("Check the format of the config file.\n");
        return 1;
    }

    if (config.mode != MODE_HUMAN_PLAYER) {
        printf("Not implemented yet.\n");
        printf("Only human player mode is currently supported.\n");
        return 1;
    }

    if (config.mode == MODE_HUMAN_PLAYER) {
        int _run = mode_run_human_player(&config);
        if (_run != 0) {
            printf("Error running game.\n");
            return 1;
        }
    }

    return 0;
}

static int mode_run_human_player(const config_t* config) {
    game_t* game = NULL;

    game_status_t _game = game_create(*config, &game);
    if (_game == GAME_STATUS_ENOMEM) {
        printf("Error creating game: out of memory.\n");
        return 1;
    }

    printf("Game created successfully.\n");

    return 0;
}
