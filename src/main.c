#include <stdio.h>
#include <string.h>

#include "config/config.h"
#include "game/game.h"

unsigned int parse_args(int argc, char** argv, config_t* config);
void print_usage();

int main(int argc, char** argv) {
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
        game_exec_status_t _game = game_play(config);
        if (_game != GAME_EXEC_STATUS_OK) {
            printf("Error during game execution.\n");
            return 1;
        }
    }

    return 0;
}

void print_usage() {
    printf("Usage: semaphore [-h [--config <path>]]\n");
}
