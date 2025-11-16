#include <stdbool.h>

#include "../lib/game_utils.h"

#define NUM_OF_ROADS 2
#define NUM_OF_TRAFFIC_LIGHTS 1

bool some_car_at_position(list_t* road, int8_t x, int8_t y) {
    for (uint8_t i = 0; i < road->size; i++) {
        car_t* car = (car_t*)list_get(road, i);

        if (car->pos_x == x && car->pos_y == y) {
            return true;
        }
    }

    return false;
}

bool some_red_tl_at_position(
    traffic_light_t* tls[],
    int8_t x,
    int8_t y,
    uint8_t orientation
) {
    for (int t = 0; t < NUM_OF_TRAFFIC_LIGHTS; t++) {
        traffic_light_t* tl = tls[t];

        if (
            orientation == ORIENTATION_HORIZONTAL &&
            tl->pos_x == x &&
            tl->pos_y == y &&
            tl->h_state == TL_RED
        ) {
            return true;
        }

        if (
            orientation == ORIENTATION_VERTICAL &&
            tl->pos_x == x &&
            tl->pos_y == y &&
            tl->v_state == TL_RED
        ) {
            return true;
        }
    }

    return false;
}
