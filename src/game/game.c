#include "./game.h"
#include "../config/config.h"

#include <stdio.h>

game_exec_status_t game_play(const config_t config) {
    printf("Playing game on default mode...\n");
    return GAME_OK;
}
