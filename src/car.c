#include <stdlib.h>
#include <stdio.h>

#include "../lib/car.h"

car_t* car_create(int x, int y) {
    car_t* car = (car_t*)malloc(sizeof(car_t));
    if (car == NULL) {
        printf("Memory allocation failed to car.\n");
        printf("Exiting...\n");
        exit(1);
    }

    car->pos_x = x;
    car->pos_y = y;
    car->cycles_waited = 0;

    return car;
}

void car_move_left(car_t* car) {
   car_move(car, -1, 0);
}

void car_move_right(car_t* car) {
    car_move(car, 1, 0);
}

void car_move_up(car_t* car) {
    car_move(car, 0, 1);
}

void car_move_down(car_t* car) {
    car_move(car, 0, -1);
}

void car_wait(car_t* car) {
    if (car == NULL) {
        return;
    }

    car->cycles_waited += 1;
}

void car_move(car_t* car, int delta_x, int delta_y) {
    if (car == NULL) {
        return;
    }

    car->pos_x += delta_x;
    car->pos_y += delta_y;
}

void car_destroy(car_t* car) {
    if (car != NULL) {
        free(car);
    }
}
