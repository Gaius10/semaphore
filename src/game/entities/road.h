#ifndef ROAD_H
#define ROAD_H

#include "./car.h"

#define ROAD_MAX_SIZE 100

typedef struct Road road_t;

typedef enum road_status {
    /** The operation completed successfully. */
    ROAD_STATUS_OK,
    /** Memory allocation failed (road_create). */
    ROAD_STATUS_ENOMEM,
    /** The road already holds ROAD_MAX_SIZE cars (road_append). */
    ROAD_STATUS_EFULL,
    /** The index is negative or not lower than the road size (road_remove, road_get). */
    ROAD_STATUS_EINDEX,
} road_status_t;

/** On success, *road holds a new road that must be released with road_destroy. */
road_status_t road_create(road_t** road);

/** The road takes ownership of the car on success. */
road_status_t road_append(road_t* road, car_t* car);
road_status_t road_remove(road_t* road, const int index);
void road_destroy(road_t** road);

/** On success, *car holds the car at index (still owned by the road). */
road_status_t road_get(const road_t* road, const int index, car_t** car);
int road_size(const road_t* road);

#endif
