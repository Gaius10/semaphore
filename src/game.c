#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

#include "../lib/list.h"
#include "../lib/game.h"
#include "../lib/input.h"
#include "../lib/traffic_light.h"
#include "../lib/game_utils.h"

static struct timespec ts_sleep = {0, 1000000000 / GAME_FREQUENCY_IN_HERTZ};

void game_init(game_t* game, struct options opts) {
    list_init(&game->road1); // horizontal
    list_init(&game->road2); // vertical

    sem_init(&game->road1_memmory, 0, 1);
    sem_init(&game->road2_memmory, 0, 1);

    tl_init(&game->traffic_light, 0, 0);

    game->status = GAME_RUNNING;
    game->score = 0;
    game->cycles_passed = 0;

    game->opts = opts;
}

void* commander(void* arg) {
    game_t* game = (game_t*)arg;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    char command;

    while (game->status == GAME_RUNNING) {
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

    if (game->opts.debug == true) {
        printf("Debugging: end of commander\n");
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

    while (game->status == GAME_RUNNING) {
        nanosleep(&ts_sleep, NULL);

        sem_wait(&game->road1_memmory);
        sem_wait(&game->road2_memmory);

        random_number = rand() % 3;

        switch (random_number) {
            case 0:
                // do nothing
                break;
            case 1:
                // push car to road 1
                car_buffer = create_car(-10, 0);
                list_append(road1, car_buffer);
                break;
            case 2:
                // push car to road 2
                car_buffer = create_car(0, -10);
                list_append(road2, car_buffer);
                break;
        }

        sem_post(&game->road1_memmory);
        sem_post(&game->road2_memmory);
    }

    if (game->opts.debug == true) {
        printf("Debugging: end of car_factory\n");
    }

    return NULL;
}

void* car_mover(void* arg) {
    game_t* game = (game_t*)arg;

    list_t* road1 = &game->road1; // horizontal
    list_t* road2 = &game->road2; // vertical

    car_t* car_buffer = NULL;

    traffic_light_t* tls[] = {&game->traffic_light};

    while (game->status == GAME_RUNNING) {
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

            if (some_green_tl_at_position(tls, car_buffer->pos_x + 1, car_buffer->pos_y, ORIENTATION_HORIZONTAL)) {
                game->score++;
            }

            car_buffer->pos_x += 1;

            if (car_buffer->pos_x > 10) {
                list_remove(road1, i);
                i--;
            }
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

            if (some_green_tl_at_position(tls, car_buffer->pos_x, car_buffer->pos_y + 1, ORIENTATION_VERTICAL)) {
                game->score++;
            }

            car_buffer->pos_y += 1;

            if (car_buffer->pos_y > 10) {
                list_remove(road2, i);
                i--;
            }
        }

        game->cycles_passed++;

        sem_post(&game->road1_memmory);
        sem_post(&game->road2_memmory);

        nanosleep(&ts_sleep, NULL);
    }

    if (game->opts.debug == true) {
        printf("Debugging: end of car_mover\n");
    }

    return NULL;
}

void* world_renderer(void* arg) {
    game_t* game = (game_t*)arg;

    list_t *road1 = &game->road1; // horizontal
    list_t *road2 = &game->road2; // vertical

    car_t* car_buffer = NULL;

    while (game->status == GAME_RUNNING) {
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

        printf(
            "Traffic Light State - Horizontal: %s | Vertical: %s\n",
            game->traffic_light.h_state == TL_GREEN ? "GREEN" : "RED",
            game->traffic_light.v_state == TL_GREEN ? "GREEN" : "RED"
        );

        printf("Score: %u | Cycles Passed: %u\n", game->score, game->cycles_passed);

        if (game->opts.debug == true) {
            for (uint8_t i = 0; i < road1->size; i++) {
                car_buffer = (car_t*)list_get(road1, i);
                printf("Road 1 - Car %d: Position X: %d | Position Y: %d\n", i, car_buffer->pos_x, car_buffer->pos_y);
            }

            for (uint8_t i = 0; i < road2->size; i++) {
                car_buffer = (car_t*)list_get(road2, i);
                printf("Road 2 - Car %d: Position X: %d | Position Y: %d\n", i, car_buffer->pos_x, car_buffer->pos_y);
            }
        }

        struct timespec ts = {0, 1000000 * 100};
        nanosleep(&ts, NULL);
    }

    if (game->opts.debug == true) {
        printf("Debugging: end of world_renderer_debugger\n");
    }

    return NULL;
}

void* game_state_manager(void* arg) {
    game_t* game = (game_t*)arg;

    while (game->status == GAME_RUNNING) {
        // Check for road overflow
        uint8_t waiting_at_road1 = count_cars_waiting(&game->road1, &game->traffic_light, ORIENTATION_HORIZONTAL);
        uint8_t waiting_at_road2 = count_cars_waiting(&game->road2, &game->traffic_light, ORIENTATION_VERTICAL);

        if (waiting_at_road1 > MAX_CARS_WAITING || waiting_at_road2 > MAX_CARS_WAITING) {
            if (game->opts.debug == true) {
                printf("Debugging: Road overflow detected (Road1: %d, Road2: %d)\n", waiting_at_road1, waiting_at_road2);
            }

            game->status = GAME_OVER;
            break;
        }

        // Check for collisions
        if (detect_collision(&game->road1, &game->road2)) {
            if (game->opts.debug == true) {
                printf("Debugging: Collision detected!\n");
            }

            game->status = GAME_OVER;
            break;
        }
    }

    if (game->opts.debug == true) {
        printf("Debugging: end of game_state_manager\n");
    }

    return NULL;
}
