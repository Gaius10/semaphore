
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <semaphore.h>

#include "list.h"
#include "traffic_light.h"

#define GAME_RUNNING 1
#define GAME_OVER 0

typedef struct game_t {
    list_t road1;
    list_t road2;
    sem_t road1_memmory;
    sem_t road2_memmory;
    traffic_light_t traffic_light;
    uint8_t status;
} game_t;

void game_init(game_t* game);

void* commander(void* arg);

void* car_factory(void* arg);
void* car_mover(void* arg);

void* world_renderer(void* arg);
void* world_renderer_debugger(void* arg);

void* game_state_manager(void* arg);

#endif
