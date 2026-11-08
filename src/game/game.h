#ifndef GAME_H
#define GAME_H

#include "../config/config.h"

typedef struct Game game_t;

typedef enum GAME_COMMAND {
    GAME_COMMAND_NOOP,
    GAME_COMMAND_HORIZONTAL_TOGGLE,
    GAME_COMMAND_VERTICAL_TOGGLE,
} game_command_t;

typedef enum {
    GAME_STATUS_RUNNING,
    GAME_STATUS_PAUSED,
    GAME_STATUS_ENDED,
} game_status_t;

game_t* game_create(const config_t config);
void game_destroy(game_t** game);

game_status_t game_get_status(const game_t* game);
game_status_t game_exec(game_t* game, game_command_t command);

#endif
