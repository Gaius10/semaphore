#ifndef CAR_H
#define CAR_H

/** Opaque type: the struct is defined in car.c. Use the functions below. */
typedef struct Car car_t;

car_t* car_create(int x, int y);

void car_move_left(car_t* car);
void car_move_right(car_t* car);
void car_move_up(car_t* car);
void car_move_down(car_t* car);
void car_wait(car_t* car);

int car_get_x(const car_t* car);
int car_get_y(const car_t* car);
unsigned int car_get_cycles_waited(const car_t* car);

void car_move(car_t* car, int delta_x, int delta_y);
void car_destroy(car_t* car);

#endif
