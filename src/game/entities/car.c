#include <stdlib.h>

#include "./car.h"

struct Car {
    int pos_x;
    int pos_y;
    unsigned int cycles_waited;
    unsigned int id;
};

car_status_t car_create(int x, int y, car_t** car) {
    car_t* new_car = malloc(sizeof(car_t));
    if (new_car == NULL) {
        return CAR_STATUS_ENOMEM;
    }

    new_car->pos_x = x;
    new_car->pos_y = y;
    new_car->cycles_waited = 0;
    new_car->id = 0;
    *car = new_car;

    return CAR_STATUS_OK;
}

void car_wait(car_t* car) {
    if (car == NULL) {
        return;
    }

    car->cycles_waited += 1;
}

unsigned int car_get_id(const car_t* car) {
    return car->id;
}

void car_set_id(car_t* car, unsigned int id) {
    if (car == NULL) {
        return;
    }

    car->id = id;
}

int car_get_x(const car_t* car) {
    return car->pos_x;
}

int car_get_y(const car_t* car) {
    return car->pos_y;
}

void car_move(car_t* car, int dx, int dy) {
    if (car == NULL) {
        return;
    }

    car->pos_x += dx;
    car->pos_y += dy;
}

void car_destroy(car_t* car) {
    if (car != NULL) {
        free(car);
    }
}
