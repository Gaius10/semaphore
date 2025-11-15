
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <semaphore.h>

#include "list.h"

typedef struct GameContext {
    List road1;
    List road2;
    sem_t road1_memmory;
    sem_t road2_memmory;
    uint8_t move_road1;
    uint8_t move_road2;
} GameArgs;

void *car_factory(void* arg);

#endif

