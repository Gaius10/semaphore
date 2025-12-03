#include <stdio.h>
#include <time.h>
#include <stddef.h>

#include "../lib/game.h"
#include "../lib/traffic_light.h"
#include "../lib/ai.h"

void* ai_commander_fixed_toggle(void* arg) {
    game_t* game = (game_t*)arg;

    tl_toggle_horizontal(&game->traffic_light);

    while (game->status == GAME_RUNNING) {
        // Simple AI logic: toggle traffic lights every 5 cycles
        if (game->cycles_passed % 5 == 0) {
            tl_toggle_horizontal(&game->traffic_light);
            tl_toggle_vertical(&game->traffic_light);
        }

        // Sleep for a short duration to simulate time passing
        struct timespec ts = {0, 500000000}; // 0.5 seconds
        nanosleep(&ts, NULL);
    }

    return NULL;
}
