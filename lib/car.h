#ifndef CAR_H
#define CAR_H

typedef struct car_t {
    int pos_x;
    int pos_y;
} car_t;

car_t* create_car(int x, int y);

void move_left(car_t* car, int distance);
void move_right(car_t* car, int distance);
void move_up(car_t* car, int distance);
void move_down(car_t* car, int distance);

void move_car(car_t* car, int delta_x, int delta_y);
void destroy_car(car_t* car);

#endif
