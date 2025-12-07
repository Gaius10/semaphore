#include "../lib/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#include "../lib/game.h"
#include "../lib/game_runner.h"
#include "../lib/ai.h"

typedef struct gamebag {
    game_t game;
    pthread_t car_factory_thread;
    pthread_t car_mover_thread;
    pthread_t world_renderer_thread;
    pthread_t commander_thread;
    pthread_t state_manager_thread;
} gamebag_t;

void gamebag_init(gamebag_t* gamebag, struct options opts);
void gamebag_join(gamebag_t* gamebag);

void run_default(struct options opts) {
    if (DEBUG) {
        printf("Starting game with the following options:\n");
        printf("Mode: %d\n", opts.mode);
        printf("Commander: %d\n", opts.commander);
        printf("Number of games: %u\n", opts.number_of_games);
        getchar();
    }

    gamebag_t* gamebag = malloc(sizeof(gamebag_t));

    gamebag_init(gamebag, opts);
    gamebag_join(gamebag);

    free(gamebag);
}

void run_performance_stats(struct options opts) {
    printf("Running in performance_stats mode.\n"); // @todo
}

void*(*renderer_factory(enum mode game_mode))(void*);
void*(*commander_factory(enum commander commander_type))(void*);

void gamebag_init(gamebag_t* gamebag, struct options opts) {
    game_init(&gamebag->game);

    pthread_create(&gamebag->car_factory_thread,    NULL, car_factory,                       &gamebag->game);
    pthread_create(&gamebag->car_mover_thread,      NULL, car_mover,                         &gamebag->game);
    pthread_create(&gamebag->state_manager_thread,  NULL, game_state_manager,                &gamebag->game);
    pthread_create(&gamebag->world_renderer_thread, NULL, renderer_factory(opts.mode),       &gamebag->game);
    pthread_create(&gamebag->commander_thread,      NULL, commander_factory(opts.commander), &gamebag->game);
}

void gamebag_join(gamebag_t* gamebag) {
    pthread_join(gamebag->car_factory_thread,    NULL);
    pthread_join(gamebag->car_mover_thread,      NULL);
    pthread_join(gamebag->state_manager_thread,  NULL);
    pthread_join(gamebag->world_renderer_thread, NULL);

    pthread_cancel(gamebag->commander_thread);
}

void*(*commander_factory(enum commander commander_type))(void*) {
    if (commander_type == COMMANDER_FIXED_TOGGLE) {
        return ai_commander_fixed_toggle;
    }

    return commander;
}

void*(*renderer_factory(enum mode game_mode))(void*) {
    if (game_mode == MODE_PERFORMANCE_STATS) {
        return world_renderer_mock;
    }

    return world_renderer;
}
