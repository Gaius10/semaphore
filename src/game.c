#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "../lib/list.h"
#include "../lib/game.h"

void* car_factory(void* arg) {
    // @todo semaphores to control access to roads memmory

    unsigned int random_number;
    srand(time(NULL));

    List *road1 = &((GameArgs*)arg)->road1; // horizontal
    List *road2 = &((GameArgs*)arg)->road2; // vertical

    Car* car_buffer = NULL;

    while (1) {
        sleep(1);
        random_number = rand() % 3;

        switch (random_number) {
            case 0:
                // do nothing
                break;
            case 1:
                // push car to stack 1
                car_buffer = create_car(-10, 0);
                list_append(road1, car_buffer);
                break;
            case 2:
                // push car to stack 2
                car_buffer = create_car(0, -10);
                list_append(road2, car_buffer);
                break;
        }
    }

    printf("Car factory is running!\n");
    printf("arg: %s\n", (char*)arg);
    return NULL;
}
