#ifndef CAR_H
#define CAR_H

typdef struct Car {
    int pos_x;
    int pos_y;
} Car;

Car* create_car(int x, int y);

void move_left(Car* car, int distance);
void move_right(Car* car, int distance);
void move_up(Car* car, int distance);
void move_down(Car* car, int distance);

void move_car(Car* car, int delta_x, int delta_y);
void destroy_car(Car* car);

#endif
