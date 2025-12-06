#ifndef CAR_H
#define CAR_H

typedef struct car_t {
    int pos_x;
    int pos_y;
    unsigned int cycles_waited;
} car_t;

car_t* car_create(int x, int y);

void car_move_left(car_t* car);
void car_move_right(car_t* car);
void car_move_up(car_t* car);
void car_move_down(car_t* car);
void car_wait(car_t* car);

void car_move(car_t* car, int delta_x, int delta_y);
void car_destroy(car_t* car);

#endif
