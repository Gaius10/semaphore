#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#define ORIENTATION_HORIZONTAL 0
#define ORIENTATION_VERTICAL 1

#include <stdbool.h>
#include "../lib/game.h"

bool some_car_at_position(list_t* roads[], int8_t x, int8_t y);
bool some_red_tl_at_position(traffic_light_t* tls[], int8_t x, int8_t y, uint8_t orientation);

#endif