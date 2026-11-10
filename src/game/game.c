#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "./game.h"
#include "./entities/road.h"
#include "./entities/traffic_light.h"
#include "../config/config.h"

struct Game {
    config_human_player_t config_human_player;
    game_state_t state;
    road_t* horizontal_road;
    road_t* vertical_road;
    traffic_light_t* horizontal_light;
    traffic_light_t* vertical_light;
    unsigned int crashes;
};

static void apply_command(game_t* game, game_command_t command);
static void move_cars_through(road_t* road, const traffic_light_t* light, int dx, int dy);
static bool road_has_car_at(const road_t* road, int x, int y);
static void factory_car(road_t* road, double spawn_probability, int x, int y);

game_status_t game_create(const config_t* config, game_t** game) {
    game_t* new_game = malloc(sizeof(game_t));
    if (new_game == NULL) {
        return GAME_STATUS_ENOMEM;
    }

    new_game->config_human_player = config_get_human_player(config);
    new_game->state = GAME_STATE_RUNNING;
    new_game->crashes = 0;

    if (road_create(&new_game->horizontal_road) != ROAD_STATUS_OK) {
        free(new_game);
        return GAME_STATUS_ENOMEM;
    }

    if (road_create(&new_game->vertical_road) != ROAD_STATUS_OK) {
        road_destroy(&new_game->horizontal_road);
        free(new_game);
        return GAME_STATUS_ENOMEM;
    }

    // The roads cross at (road_length_h, road_length_v). Each light stops its cars one cell before it.
    unsigned int rlen_h = new_game->config_human_player.road_length_h;
    unsigned int rlen_v = new_game->config_human_player.road_length_v;
    traffic_light_color_t green = TRAFFIC_LIGHT_GREEN;
    traffic_light_status_t _light;
    
    _light = traffic_light_create(rlen_h - 1, rlen_v, green, &new_game->horizontal_light);
    if (_light != TRAFFIC_LIGHT_STATUS_OK) {
        road_destroy(&new_game->vertical_road);
        road_destroy(&new_game->horizontal_road);
        free(new_game);
        return GAME_STATUS_ENOMEM;
    }

    _light = traffic_light_create(rlen_h, rlen_v - 1, green, &new_game->vertical_light);
    if (_light != TRAFFIC_LIGHT_STATUS_OK) {
        traffic_light_destroy(&new_game->horizontal_light);
        road_destroy(&new_game->vertical_road);
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
    traffic_light_destroy(&(*game)->horizontal_light);
    traffic_light_destroy(&(*game)->vertical_light);
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

const traffic_light_t* game_get_horizontal_light(const game_t* game) {
    return game->horizontal_light;
}

const traffic_light_t* game_get_vertical_light(const game_t* game) {
    return game->vertical_light;
}

unsigned int game_get_crashes(const game_t* game) {
    return game->crashes;
}

game_state_t game_exec(game_t* game, game_command_t command) {
    if (game->state == GAME_STATE_ENDED) {
        return game->state;
    }

    apply_command(game, command);

    move_cars_through(game->horizontal_road, game->horizontal_light, 1, 0);
    move_cars_through(game->vertical_road, game->vertical_light, 0, 1);

    double spawn_probability_h = game->config_human_player.car_spawn_probability_h;
    double spawn_probability_v = game->config_human_player.car_spawn_probability_v;
    unsigned int road_length_h = game->config_human_player.road_length_h;
    unsigned int road_length_v = game->config_human_player.road_length_v;

    if (road_has_car_at(game->horizontal_road, road_length_h, road_length_v)
            && road_has_car_at(game->vertical_road, road_length_h, road_length_v)) {
        game->crashes++;
    }

    factory_car(game->horizontal_road, spawn_probability_h, 0, road_length_v);
    factory_car(game->vertical_road, spawn_probability_v, road_length_h, 0);

    return game->state;
}

static void apply_command(game_t* game, game_command_t command) {
    switch (command) {
        case GAME_COMMAND_HORIZONTAL_TOGGLE:
            traffic_light_toggle(game->horizontal_light);
            break;
        case GAME_COMMAND_VERTICAL_TOGGLE:
            traffic_light_toggle(game->vertical_light);
            break;
        case GAME_COMMAND_NOOP:
            break;
    }
}

/**
 * The road holds its cars in spawn order, so each car is right behind the one before it in the
 * road. Going front to back, the car ahead has already moved (or waited) when a car checks
 * whether the cell it wants is free.
 */
static void move_cars_through(road_t* road, const traffic_light_t* light, int dx, int dy) {
    car_t* ahead = NULL;

    for (int i = 0; i < road_size(road); i++) {
        car_t* car = NULL;
        if (road_get(road, i, &car) != ROAD_STATUS_OK) {
            continue;
        }

        int x = car_get_x(car);
        int y = car_get_y(car);
        bool held = traffic_light_holds(light, x, y);
        bool blocked = ahead != NULL && car_get_x(ahead) == x + dx && car_get_y(ahead) == y + dy;

        if (held || blocked) {
            car_wait(car);
        } else {
            car_move(car, dx, dy);
        }

        ahead = car;
    }
}

static bool road_has_car_at(const road_t* road, int x, int y) {
    for (int i = 0; i < road_size(road); i++) {
        car_t* car = NULL;
        if (road_get(road, i, &car) == ROAD_STATUS_OK && car_get_x(car) == x && car_get_y(car) == y) {
            return true;
        }
    }

    return false;
}

static void factory_car(road_t* road, double spawn_probability, int x, int y) {
    if (road_has_car_at(road, x, y)) {
        return; // The queue reaches back to the spawn cell: the road is full
    }

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
