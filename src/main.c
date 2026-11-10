#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "config/config.h"
#include "game/game.h"
#include "game/view.h"

static int run(const config_t* config);
static int mode_run_human_player(const config_t* config);

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

static int mode_run_human_player(const config_t* config) {
    game_t* game = NULL;

    game_status_t _game = game_create(config, &game);
    if (_game == GAME_STATUS_ENOMEM) {
        printf("Error creating game: out of memory.\n");
        return 1;
    }

    config_view_t view_config = config_get_view(config);
    view_t* view = NULL;
    view_status_t _view = view_create(stdout, view_config.width, view_config.height, &view);
    if (_view != VIEW_STATUS_OK) {
        printf("Error creating view.\n");
        game_destroy(&game);
        return 1;
    }

    int i = 0;
    while (i++ < 10) {
        game_exec(game, GAME_COMMAND_NOOP);
        view_render(view, game_get_horizontal_road(game), game_get_vertical_road(game));
        sleep(1);
    }

    view_destroy(&view);
    game_destroy(&game);
    return 0;
}
