
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <semaphore.h>
#include <stdbool.h>

#include "list.h"
#include "traffic_light.h"

#define GAME_RUNNING 1
#define GAME_OVER 0

enum game_over_reason {
    GAME_OVER_NONE,
    GAME_OVER_ROAD_OVERFLOW,
    GAME_OVER_COLLISION
};

typedef struct game_t {
    list_t road1;
    list_t road2;
    sem_t road1_memmory;
    sem_t road2_memmory;
    traffic_light_t traffic_light;
    uint8_t status;

    struct stats {
        unsigned int cars_created;
        unsigned int cars_passed;
        unsigned int average_wait_cycles;
        unsigned int cycles_passed;
        enum game_over_reason game_over_reason;
    } stats;
} game_t;

void game_init(game_t* game);
void game_update_stats(game_t* game);

/**
 * Thread functions
 */

void* commander(void* arg);

void* car_factory(void* arg);
void* car_mover(void* arg);

void* world_renderer(void* arg);
void* world_renderer_debugger(void* arg);

void* game_state_manager(void* arg);

#endif
