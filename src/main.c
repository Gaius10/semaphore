#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config/config.h"
#include "mode/human_player.h"

static int run(const config_t* config);

int main(int argc, char** argv) {
    srand((unsigned int) time(NULL));

    config_t* config = NULL;
    config_status_t _config = config_create("config.ini", &config);
    if (_config == CONFIG_STATUS_ENOMEM) {
        printf("Error creating config: out of memory.\n");
        return 1;
    }

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

    if (_config == CONFIG_STATUS_EMISSING) {
        printf("Missing a required key in the config file.\n");
        printf("Check that 'mode' and the settings for that mode are all set.\n");
        return 1;
    }

    int exit_code = run(config);

    config_destroy(&config);
    return exit_code;
}

static int run(const config_t* config) {
    if (config_get_mode(config) != MODE_HUMAN_PLAYER) {
        printf("Not implemented yet.\n");
        printf("Only human player mode is currently supported.\n");
        return 1;
    }

    int _run = mode_run_human_player(config);
    if (_run != 0) {
        printf("Error running game.\n");
        return 1;
    }

    return 0;
}
