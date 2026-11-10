#ifndef GAME_H
#define GAME_H

#include "../config/config.h"
#include "./entities/road.h"

typedef struct Game game_t;

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

game_state_t game_exec(game_t* game, game_command_t command);

#endif
