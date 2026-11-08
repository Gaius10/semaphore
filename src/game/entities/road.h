#ifndef ROAD_H
#define ROAD_H

#include "./car.h"

#define ROAD_MAX_SIZE 100

typedef struct road_t {
    car_t* items[ROAD_MAX_SIZE];
    int size;
} road_t;

void road_init(road_t* road);

void road_append(road_t* road, car_t* car);
void road_remove(road_t* road, int index);
void road_destroy(road_t** road);

car_t* road_get(road_t* road, int index);

#endif
