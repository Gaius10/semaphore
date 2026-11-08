#ifndef ROAD_H
#define ROAD_H

#include "./car.h"

#define ROAD_MAX_SIZE 100


typedef struct Road road_t;

road_t* road_create(void);

void road_append(road_t* road, const car_t* car);
void road_remove(road_t* road, const int index);
void road_destroy(road_t** road);

car_t* road_get(const road_t* road, const int index);
int road_size(const road_t* road);

#endif
