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

bool some_green_tl_at_position(
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
            tl->h_state == TL_GREEN
        ) {
            return true;
        }

        if (
            orientation == ORIENTATION_VERTICAL &&
            tl->pos_x == x &&
            tl->pos_y == y &&
            tl->v_state == TL_GREEN
        ) {
            return true;
        }
    }

    return false;
}

uint8_t count_cars_waiting(
    list_t* road,
    traffic_light_t* tl,
    uint8_t orientation
) {
    uint8_t count = 0;

    for (uint8_t i = 0; i < road->size; i++) {
        car_t* car = (car_t*)list_get(road, i);

        if (
            (orientation == ORIENTATION_HORIZONTAL && car->pos_x < tl->pos_x) ||
            (orientation == ORIENTATION_VERTICAL && car->pos_y < tl->pos_y)
        ) {
            count++;
        }
    }

    return count;
}

bool detect_collision(list_t* road1, list_t* road2) {
    uint8_t cars_at_origin = 0;

    for (uint8_t i = 0; i < road1->size; i++) {
        car_t* car1 = (car_t*)list_get(road1, i);

        if (car1->pos_x == 0 && car1->pos_y == 0) {
            cars_at_origin++;
        }
    }

    for (uint8_t j = 0; j < road2->size; j++) {
        car_t* car2 = (car_t*)list_get(road2, j);

        if (car2->pos_x == 0 && car2->pos_y == 0) {
            cars_at_origin++;
        }
    }

    return cars_at_origin > 1;
}
