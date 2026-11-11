#ifndef GAME_H
#define GAME_H

#include "../config/config.h"
#include "./entities/road.h"
#include "./entities/traffic_light.h"

typedef struct Game game_t;

/** What the player does on a tick. Each toggle flips the light of its road. */
typedef enum GAME_COMMAND {
    GAME_COMMAND_NOOP,
    GAME_COMMAND_HORIZONTAL_TOGGLE,
    GAME_COMMAND_VERTICAL_TOGGLE,
} game_command_t;

typedef enum {
    GAME_STATE_RUNNING,
    GAME_STATE_ENDED,
} game_state_t;

/** Outcome of a game operation (the game's own state is game_state_t). */
typedef enum game_status {
    /** The operation completed successfully. */
    GAME_STATUS_OK,
    /** Memory allocation failed (game_create). */
    GAME_STATUS_ENOMEM,
} game_status_t;

/**
 * On success, *game holds a new game that must be released with game_destroy.
 * The game copies what it needs from the config, so the config may be destroyed right away.
 */
game_status_t game_create(const config_t* config, game_t** game);
void game_destroy(game_t** game);

game_state_t game_get_state(const game_t* game);

/**
 * Read-only access to the game's own roads, e.g. to hand them to a view.
 * The pointers stay owned by the game and are only valid until the next game_exec or game_destroy.
 */
const road_t* game_get_horizontal_road(const game_t* game);
const road_t* game_get_vertical_road(const game_t* game);

/**
 * Read-only access to the roads' traffic lights. Like the roads, the pointers stay owned by the
 * game and are valid until game_destroy. The horizontal light starts green and the vertical one
 * starts red. The horizontal stop cell is (road_length_h - 1, road_length_v) and the vertical
 * one is (road_length_h, road_length_v - 1), right before the crossing at
 * (road_length_h, road_length_v).
 */
const traffic_light_t* game_get_horizontal_light(const game_t* game);
const traffic_light_t* game_get_vertical_light(const game_t* game);

/** How many ticks ended with cars of both roads in the crossing. Crashes do not end the game. */
unsigned int game_get_crashes(const game_t* game);

/** How many cars have passed a traffic light: a car counts once, when it moves out of its light's stop cell. Starts at 0. */
unsigned int game_get_cars_passed(const game_t* game);

/**
 * Runs one tick:
 * 1. Applies the command (a toggle flips the light of its road).
 * 2. Moves every car one cell along its road. A car waits instead when its light holds it
 *    (red, and the car is on the stop cell) or when the car ahead of it is in the way.
 * 3. Counts a crash if both roads have a car in the crossing.
 * 4. Spawns new cars, unless the road's spawn cell is taken by a car that is waiting.
 */
game_state_t game_exec(game_t* game, game_command_t command);

#endif
