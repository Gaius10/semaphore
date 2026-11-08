#include <stdlib.h>

#include "./game.h"
#include "./entities/road.h"
#include "../config/config.h"

struct Game {
    config_t config;
    game_state_t state;
    road_t* horizontal_road;
    road_t* vertical_road;
};

game_status_t game_create(const config_t config, game_t** game) {
    game_t* new_game = malloc(sizeof(game_t));
    if (new_game == NULL) {
        return GAME_STATUS_ENOMEM;
    }

    new_game->config = config;
    new_game->state = GAME_STATE_RUNNING;

    if (road_create(&new_game->horizontal_road) != ROAD_STATUS_OK) {
        free(new_game);
        return GAME_STATUS_ENOMEM;
    }

    if (road_create(&new_game->vertical_road) != ROAD_STATUS_OK) {
        road_destroy(&new_game->horizontal_road);
        free(new_game);
        return GAME_STATUS_ENOMEM;
    }

    *game = new_game;

    return GAME_STATUS_OK;
}

void game_destroy(game_t** game) {
    if (!game || !*game) {
        return;
    }

    road_destroy(&(*game)->horizontal_road);
    road_destroy(&(*game)->vertical_road);
    free(*game);
    *game = NULL;
}

game_state_t game_get_state(const game_t* game) {
    if (!game) {
        return GAME_STATE_ENDED;
    }

    return game->state;
}

game_state_t game_exec(game_t* game, game_command_t command) {
    if (game->state == GAME_STATE_ENDED) {
        return game->state;
    }

    switch (command) {
        case GAME_COMMAND_NOOP:
            break;
        case GAME_COMMAND_HORIZONTAL_TOGGLE:
            break;
        case GAME_COMMAND_VERTICAL_TOGGLE:
            break;
        default:
            break;
    }

    return game->state;
}
