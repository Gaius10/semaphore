
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <semaphore.h>

#include "list.h"

typedef struct game_t {
    list_t road1;
    list_t road2;
    sem_t road1_memmory;
    sem_t road2_memmory;
    uint8_t move_road1;
    uint8_t move_road2;
} game_t;

void game_init(game_t* game);

void* car_factory(void* arg);
void* car_mover(void* arg);
void* world_renderer(void* arg);

#endif

