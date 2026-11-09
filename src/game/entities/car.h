#ifndef CAR_H
#define CAR_H

/** Opaque type: the struct is defined in car.c. Use the functions below. */
typedef struct Car car_t;

typedef enum car_status {
    /** The operation completed successfully. */
    CAR_STATUS_OK,
    /** Memory allocation failed (car_create). */
    CAR_STATUS_ENOMEM,
} car_status_t;

/** On success, *car holds a new car that must be released with car_destroy. */
car_status_t car_create(int x, int y, car_t** car);

void car_wait(car_t* car);

int car_get_x(const car_t* car);
int car_get_y(const car_t* car);

void car_move(car_t* car, int delta_x, int delta_y);
void car_destroy(car_t* car);

#endif
