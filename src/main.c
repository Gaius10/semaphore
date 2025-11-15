#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "../lib/game.h"

int main() {
    game_t game;

    pthread_t car_factory_thread;
    pthread_t car_mover_thread;
    pthread_t world_renderer_thread;
    pthread_t commander_thread;

    game_init(&game);

    pthread_create(&car_factory_thread, NULL, car_factory, &game);
    pthread_create(&car_mover_thread, NULL, car_mover, &game);
    pthread_create(&world_renderer_thread, NULL, world_renderer, &game);
    pthread_create(&commander_thread, NULL, commander, &game);

    pthread_join(car_factory_thread, NULL);
    pthread_join(car_mover_thread, NULL);
    pthread_join(world_renderer_thread, NULL);
    pthread_join(commander_thread, NULL);

    return 0;
}
