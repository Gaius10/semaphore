#include <stdlib.h>
#include <stdio.h>

#include "../lib/car.h"

car_t* create_car(int x, int y) {
    car_t* car = (car_t*)malloc(sizeof(car_t));
    if (car == NULL) {
        printf("Memory allocation failed to car.\n");
        printf("Exiting...\n");
        exit(1);
    }

    car->pos_x = x;
    car->pos_y = y;

    return car;
}

void move_left(car_t* car, int distance) {
    if (car == NULL) {
        return;
    }

    car->pos_x -= distance;
}

void move_right(car_t* car, int distance) {
    if (car == NULL) {
        return;
    }

    car->pos_x += distance;
}

void move_up(car_t* car, int distance) {
    if (car == NULL) {
        return;
    }

    car->pos_y += distance;
}

void move_down(car_t* car, int distance) {
    if (car == NULL) {
        return;
    }

    car->pos_y -= distance;
}

void move_car(car_t* car, int delta_x, int delta_y) {
    if (car == NULL) {
        return;
    }

    car->pos_x += delta_x;
    car->pos_y += delta_y;
}

void destroy_car(car_t* car) {
    if (car != NULL) {
        free(car);
    }
}
