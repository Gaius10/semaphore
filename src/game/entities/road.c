#include <stdlib.h>
#include <stdio.h>

#include "./road.h"
#include "./car.h"

struct Road {
    car_t* items[ROAD_MAX_SIZE];
    int size;
};

road_t* road_create(void) {
    road_t* road = malloc(sizeof(road_t));
    if (road == NULL) {
        printf("Memory allocation failed to road.\n");
        printf("Exiting...\n");
        exit(1);
    }

    for (unsigned i = 0; i < ROAD_MAX_SIZE; i++) {
        road->items[i] = NULL;
    }

    road->size = 0;

    return road;
}

void road_append(road_t* road, car_t* car) {
    if (road->size >= ROAD_MAX_SIZE) {
        printf("road_t is full. Cannot append new car.\n");
        return;
    }

    road->items[road->size++] = car;
}

void road_remove(road_t* road, int index) {
    if (index < 0 || index >= road->size) {
        printf("Index out of bounds. Cannot remove car.\n");
        return;
    }

    car_destroy(road->items[index]);
    for (int i = index; i < road->size - 1; i++) {
        road->items[i] = road->items[i + 1];
    }

    road->size--;
}

void road_destroy(road_t** road) {
    for (int i = 0; i < (*road)->size; i++) {
        car_destroy((*road)->items[i]);
    }

    (*road)->size = 0;
    free(*road);
    *road = NULL;
}

car_t* road_get(const road_t* road, int index) {
    if (index < 0 || index >= road->size) {
        printf("Index out of bounds. Cannot get car.\n");
        return NULL;
    }

    return road->items[index];
}

int road_size(const road_t* road) {
    return road->size;
}
