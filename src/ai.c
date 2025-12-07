#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stddef.h>

#include "../lib/game.h"
#include "../lib/traffic_light.h"
#include "../lib/ai.h"

// Just a convenient helper function
void wait();

void* ai_commander_fixed_toggle(void* arg) {
    game_t* game = (game_t*)arg;

    tl_toggle_horizontal(&game->traffic_light);

    while (game->status == GAME_WAITING) {
        sched_yield();
    }

    while (game->status == GAME_RUNNING) {
        // Simple AI logic: toggle traffic lights every 5 cycles
        if (game->stats.cycles_passed % 5 == 0) {
            tl_toggle_horizontal(&game->traffic_light);
            tl_toggle_vertical(&game->traffic_light);
        }

        wait();
    }

    return NULL;
}

void* ai_commander_random_toggle(void* arg) {
    game_t* game = (game_t*) arg;

    srand((unsigned) time(NULL));

    while (game->status == GAME_WAITING) {
        sched_yield();
    }

    while (game->status == GAME_RUNNING) {
        if (game->stats.cycles_passed % 4 != 0) {
            wait();
            continue;
        }

        int random_number = rand();

        if (random_number % 2 == 0) {
            tl_toggle_horizontal(&game->traffic_light);
        }

        if (random_number % 3 == 0) {
            tl_toggle_vertical(&game->traffic_light);
        }

        wait();
    }

    return NULL;
}

void wait() {
    // Sleep for a short duration to simulate time passing
    struct timespec ts = {0, 500000000}; // 0.5 seconds
    nanosleep(&ts, NULL);
}
