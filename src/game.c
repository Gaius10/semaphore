#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

#include "../lib/list.h"
#include "../lib/game.h"

void game_init(game_t* game) {
    list_init(&game->road1); // horizontal
    list_init(&game->road2); // vertical

    sem_init(&game->road1_memmory, 0, 1);
    sem_init(&game->road2_memmory, 0, 1);

    game->move_road1 = 0;
    game->move_road2 = 0;
}

void* car_factory(void* arg) {
    // @todo semaphores to control access to roads memmory

    unsigned int random_number;
    srand(time(NULL));

    game_t* game = (game_t*)arg;

    list_t *road1 = &game->road1; // horizontal
    list_t *road2 = &game->road2; // vertical

    car_t* car_buffer = NULL;

    while (1) {
        sleep(1);

        sem_wait(&game->road1_memmory);
        sem_wait(&game->road2_memmory);

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

        sem_post(&game->road1_memmory);
        sem_post(&game->road2_memmory);
    }

    return NULL;
}

void* car_mover(void* arg) {
    // @todo
    printf("Car mover is running!\n");
    printf("arg: %s\n", (char*)arg);
    return NULL;
}

void* world_renderer(void* arg) {
    while (1) {
        system("clear");

        for (uint8_t i = 0; i < 40; i++) {
            for (uint8_t j = 0; j < 40; j++) {
                if (i == 19 || j == 19) {
                    printf("+");
                } else {
                    printf(" ");
                }
            }
            printf("\n");
        }

        sleep(1);
    }

    return NULL;
}
