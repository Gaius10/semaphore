#include "./game.h"
#include "./entities/road.h"
#include "../config/config.h"

#include <pthread.h>

struct Game {
    config_t* config;
    game_status_t status;
}

game_t* game_create(const config_t config) {
    game_t* game = malloc(sizeof(game_t));
    if (!game) {
        return NULL;
    }

    game->config = config;
    game->status = GAME_STATUS_RUNNING;
    game->horizontal_road = road_create(config.road_length, config.road_width, config.horizontal_road_speed);

    return game;
}
