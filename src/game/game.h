#ifndef GAME_H
#define GAME_H

#include "../config/config.h"

typedef enum game_exec_status {
    GAME_OK
} game_exec_status_t;

game_exec_status_t game_play(const config_t config);

#endif
