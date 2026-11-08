#include <stdlib.h>

#include "./road.h"
#include "./car.h"

struct Road {
    car_t* items[ROAD_MAX_SIZE];
    int size;
};

road_status_t road_create(road_t** road) {
    road_t* new_road = malloc(sizeof(road_t));
    if (new_road == NULL) {
        return ROAD_STATUS_ENOMEM;
    }

    for (unsigned i = 0; i < ROAD_MAX_SIZE; i++) {
        new_road->items[i] = NULL;
    }

    new_road->size = 0;
    *road = new_road;

    return ROAD_STATUS_OK;
}

road_status_t road_append(road_t* road, car_t* car) {
    if (road->size >= ROAD_MAX_SIZE) {
        return ROAD_STATUS_EFULL;
    }

    road->items[road->size++] = car;

    return ROAD_STATUS_OK;
}

road_status_t road_remove(road_t* road, const int index) {
    if (index < 0 || index >= road->size) {
        return ROAD_STATUS_EINDEX;
    }

    car_destroy(road->items[index]);
    for (int i = index; i < road->size - 1; i++) {
        road->items[i] = road->items[i + 1];
    }

    road->size--;

    return ROAD_STATUS_OK;
}

void road_destroy(road_t** road) {
    for (int i = 0; i < (*road)->size; i++) {
        car_destroy((*road)->items[i]);
    }

    (*road)->size = 0;
    free(*road);
    *road = NULL;
}

road_status_t road_get(const road_t* road, const int index, car_t** car) {
    if (index < 0 || index >= road->size) {
        return ROAD_STATUS_EINDEX;
    }

    *car = road->items[index];

    return ROAD_STATUS_OK;
}

int road_size(const road_t* road) {
    return road->size;
}
