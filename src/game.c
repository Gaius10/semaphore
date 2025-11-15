#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

#include "../lib/list.h"
#include "../lib/game.h"
#include "../lib/input.h"

void game_init(game_t* game) {
    list_init(&game->road1); // horizontal
    list_init(&game->road2); // vertical

    sem_init(&game->road1_memmory, 0, 1);
    sem_init(&game->road2_memmory, 0, 1);

    game->move_road1 = 0;
    game->move_road2 = 0;
}

void* commander(void* arg) {
    game_t* game = (game_t*)arg;
    char command;

    while (1) {
        command = getch();
        switch (command) {
            case 'h':
            case 'H':
                game->move_road1 = !game->move_road1;
                break;
            case 'v':
            case 'V':
                game->move_road2 = !game->move_road2;
                break;
            default:
                break;
        }
    }

    return NULL;
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
    game_t* game = (game_t*)arg;

    list_t *road1 = &game->road1; // horizontal
    list_t *road2 = &game->road2; // vertical

    car_t* car_buffer = NULL;

    while (1) {
        sem_wait(&game->road1_memmory);
        sem_wait(&game->road2_memmory);

        if (game->move_road1) {
            for (uint8_t i = 0; i < road1->size; i++) {
                car_buffer = (car_t*)list_get(road1, i);
                car_buffer->pos_x += 1;
            }
        }

        if (game->move_road2) {
            for (uint8_t i = 0; i < road2->size; i++) {
                car_buffer = (car_t*)list_get(road2, i);
                car_buffer->pos_y += 1;
            }
        }

        sem_post(&game->road1_memmory);
        sem_post(&game->road2_memmory);

        sleep(1);
    }

    return NULL;
}

void* world_renderer(void* arg) {
    game_t* game = (game_t*)arg;

    list_t *road1 = &game->road1; // horizontal
    list_t *road2 = &game->road2; // vertical

    car_t* car_buffer = NULL;

    while (1) {
        system("clear");

        sem_wait(&game->road1_memmory);
        sem_wait(&game->road2_memmory);

        for (int8_t i = 10; i >= -10; i--) {
            for (int8_t j = -10; j <= 10; j++) {
                uint8_t drawn = 0;

                // check road 1
                for (uint8_t k = 0; k < road1->size; k++) {
                    car_buffer = (car_t*)list_get(road1, k);
                    if (car_buffer->pos_x == j && car_buffer->pos_y == i) {
                        printf(">");
                        drawn = 1;
                        break;
                    }
                }

                // check road 2
                for (uint8_t k = 0; k < road2->size; k++) {
                    car_buffer = (car_t*)list_get(road2, k);
                    if (car_buffer->pos_x == j && car_buffer->pos_y == i) {
                        printf("^");
                        drawn = 1;
                        break;
                    }
                }

                if (!drawn) {
                    printf(" ");
                }
            }

            printf("\n");
        }

        sem_post(&game->road1_memmory);
        sem_post(&game->road2_memmory);

        sleep(1);
    }

    return NULL;
}

void* world_renderer_debugger(void* arg) {
    game_t* game = (game_t*)arg;

    list_t *road1 = &game->road1; // horizontal
    list_t *road2 = &game->road2; // vertical

    car_t* car_buffer = NULL;

    while (1) {
        system("clear");

        sem_wait(&game->road1_memmory);
        sem_wait(&game->road2_memmory);

        for (int8_t i = 10; i >= -10; i--) {
            for (int8_t j = -10; j <= 10; j++) {
                uint8_t drawn = 0;

                // check road 1
                for (uint8_t k = 0; k < road1->size; k++) {
                    car_buffer = (car_t*)list_get(road1, k);
                    if (car_buffer->pos_x == j && car_buffer->pos_y == i) {
                        printf(">");
                        drawn = 1;
                        break;
                    }
                }

                // check road 2
                for (uint8_t k = 0; k < road2->size; k++) {
                    car_buffer = (car_t*)list_get(road2, k);
                    if (car_buffer->pos_x == j && car_buffer->pos_y == i) {
                        printf("^");
                        drawn = 1;
                        break;
                    }
                }

                if (!drawn) {
                    printf(" ");
                }
            }

            printf("\n");
        }

        for (uint8_t i = 0; i < road1->size; i++) {
            car_buffer = (car_t*)list_get(road1, i);
            printf("Road 1 - Car %d: Position X: %d | Position Y: %d\n", i, car_buffer->pos_x, car_buffer->pos_y);
        }

        for (uint8_t i = 0; i < road2->size; i++) {
            car_buffer = (car_t*)list_get(road2, i);
            printf("Road 2 - Car %d: Position X: %d | Position Y: %d\n", i, car_buffer->pos_x, car_buffer->pos_y);
        }

        sem_post(&game->road1_memmory);
        sem_post(&game->road2_memmory);

        sleep(1);
    }

    return NULL;
}
