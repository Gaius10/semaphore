#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

#include "../lib/list.h"
#include "../lib/game.h"
#include "../lib/input.h"
#include "../lib/traffic_light.h"
#include "../lib/game_utils.h"

void game_init(game_t* game) {
    list_init(&game->road1); // horizontal
    list_init(&game->road2); // vertical

    sem_init(&game->road1_memmory, 0, 1);
    sem_init(&game->road2_memmory, 0, 1);

    tl_init(&game->traffic_light, 0, 0);
}

void* commander(void* arg) {
    game_t* game = (game_t*)arg;
    char command;

    while (1) {
        command = getch();
        switch (command) {
            case 'h':
            case 'H':
                tl_toggle_horizontal(&game->traffic_light);
                break;
            case 'v':
            case 'V':
                tl_toggle_vertical(&game->traffic_light);
                break;
            default:
                break;
        }
    }

    return NULL;
}

void* car_factory(void* arg) {
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

    list_t* road1 = &game->road1; // horizontal
    list_t* road2 = &game->road2; // vertical

    car_t* car_buffer = NULL;

    traffic_light_t* tls[] = {&game->traffic_light};

    while (1) {
        sem_wait(&game->road1_memmory);
        sem_wait(&game->road2_memmory);

        // Process road 1
        for (uint8_t i = 0; i < road1->size; i++) {
            car_buffer = (car_t*)list_get(road1, i);

            if (some_car_at_position(road1, car_buffer->pos_x + 1, car_buffer->pos_y)) {
                continue;
            }

            if (some_red_tl_at_position(tls, car_buffer->pos_x + 1, car_buffer->pos_y, ORIENTATION_HORIZONTAL)) {
                continue;
            }

            car_buffer->pos_x += 1;
        }

        // Process road 2
        for (uint8_t i = 0; i < road2->size; i++) {
            car_buffer = (car_t*)list_get(road2, i);

            if (some_car_at_position(road2, car_buffer->pos_x, car_buffer->pos_y + 1)) {
                continue;
            }

            if (some_red_tl_at_position(tls, car_buffer->pos_x, car_buffer->pos_y + 1, ORIENTATION_VERTICAL)) {
                continue;
            }

            car_buffer->pos_y += 1;
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

        printf("Traffic Light State - Horizontal: %s | Vertical: %s\n",
               game->traffic_light.h_state == TL_GREEN ? "GREEN" : "RED",
               game->traffic_light.v_state == TL_GREEN ? "GREEN" : "RED");

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
