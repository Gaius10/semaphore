#include <stdlib.h>
#include <stdio.h>

#include "./game.h"
#include "./entities/road.h"
#include "../config/config.h"

struct Game {
    config_human_player_t config_human_player;
    game_state_t state;
    road_t* horizontal_road;
    road_t* vertical_road;
};

game_status_t game_create(const config_t* config, game_t** game) {
    game_t* new_game = malloc(sizeof(game_t));
    if (new_game == NULL) {
        return GAME_STATUS_ENOMEM;
    }

    new_game->config_human_player = config_get_human_player(config);
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

const road_t* game_get_horizontal_road(const game_t* game) {
    return game->horizontal_road;
}

const road_t* game_get_vertical_road(const game_t* game) {
    return game->vertical_road;
}

void move_cars_through(road_t* road, int dx, int dy);
void factory_car(road_t* road, double spawn_probability, int x, int y);
game_state_t game_exec(game_t* game, game_command_t command) {
    if (game->state == GAME_STATE_ENDED) {
        return game->state;
    }

    move_cars_through(game->horizontal_road, 1, 0);
    move_cars_through(game->vertical_road, 0, 1);

    double spawn_probability_h = game->config_human_player.car_spawn_probability_h;
    double spawn_probability_v = game->config_human_player.car_spawn_probability_v;
    int road_length_h = game->config_human_player.road_length_h;
    int road_length_v = game->config_human_player.road_length_v;

    factory_car(game->horizontal_road, spawn_probability_h, 0, road_length_v);
    factory_car(game->vertical_road, spawn_probability_v, road_length_h, 0);

    return game->state;
}

void move_cars_through(road_t* road, int dx, int dy) {
    for (int i = 0; i < road_size(road); i++) {
        car_t* car;

        road_status_t _road = road_get(road, i, &car);
        if (_road != ROAD_STATUS_OK) {
            continue;
        }

        car_move(car, dx, dy);
    }
}

void factory_car(road_t* road, double spawn_probability, int x, int y) {
    if ((double)rand() / RAND_MAX < spawn_probability) {
        car_t* new_car = NULL;
        car_status_t _car = car_create(x, y, &new_car);
        if (_car != CAR_STATUS_OK) {
            // Think about how to handle this error.
            // For now, we just print an error message and return.
            // I'm not sure if this is the best way to handle this, but it's a start.
            fprintf(stderr, "Error creating car: %d\n", _car);
            return;
        }

        road_status_t _road = road_append(road, new_car);
        if (_road != ROAD_STATUS_OK) {
            // Think about how to handle this error.
            // For now, we just print an error message and return.
            // I'm not sure if this is the best way to handle this, but it's a start.
            fprintf(stderr, "Error appending car to road: %d\n", _road);
            car_destroy(new_car);
            return;
        }
    }
}
