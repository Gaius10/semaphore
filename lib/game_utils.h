#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#define ORIENTATION_HORIZONTAL 0
#define ORIENTATION_VERTICAL 1

#include <stdbool.h>
#include "../lib/game.h"

bool some_car_at_position(list_t* road, int8_t x, int8_t y);
bool some_red_tl_at_position(traffic_light_t* tls[], int8_t x, int8_t y, uint8_t orientation);

uint8_t count_cars_waiting(list_t* road, traffic_light_t* tl, uint8_t orientation);
bool detect_collision(list_t* road1, list_t* road2);

#endif