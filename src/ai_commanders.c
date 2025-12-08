#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stddef.h>
#include <math.h>

#include "../lib/game.h"
#include "../lib/traffic_light.h"
#include "../lib/ai_commanders.h"

// Just a convenient helper function
void wait();

void* ai_commander_fixed_toggle(void* arg) {
    game_t* game = (game_t*)arg;

    tl_toggle_horizontal(&game->traffic_light);

    while (game->status == GAME_WAITING) {
        sched_yield();
    }

    while (game->status == GAME_RUNNING) {
        // Simple AI logic: toggle traffic lights every 5 cycles
        if (game->stats.cycles_passed % 5 == 0) {
            tl_toggle_horizontal(&game->traffic_light);
            tl_toggle_vertical(&game->traffic_light);
        }

        wait();
    }

    return NULL;
}

void* ai_commander_random_toggle(void* arg) {
    game_t* game = (game_t*) arg;

    srand((unsigned) time(NULL));

    while (game->status == GAME_WAITING) {
        sched_yield();
    }

    while (game->status == GAME_RUNNING) {
        if (game->stats.cycles_passed % 4 != 0) {
            wait();
            continue;
        }

        int random_number = rand();

        if (random_number % 2 == 0) {
            tl_toggle_horizontal(&game->traffic_light);
        }

        if (random_number % 3 == 0) {
            tl_toggle_vertical(&game->traffic_light);
        }

        wait();
    }

    return NULL;
}

double avg_origin_distance(list_t* road);
void apply_signals(traffic_light_t* tl, bool horizontal, bool vertical);
void* ai_commander_specimen_model_01(void* arg) {
    game_t* game = (game_t*) arg;

    double avg_distance1;
    double avg_distance2;

    bool signal_horizontal;
    bool signal_vertical;

    double road1_a, road1_b;
    double road2_a, road2_b;

    FILE* model_file = fopen("model.cfg", "r");

    if (fscanf(model_file, "%lf %lf %lf %lf", &road1_a, &road1_b, &road2_a, &road2_b) != 4) {
        fprintf(stderr, "Error reading model file.\n");
        exit(EXIT_FAILURE);
    }

    fclose(model_file);

    while (game->status == GAME_WAITING) {
        sched_yield();
    }

    FILE* output_file = fopen("output.txt", "w");
    while (game->status == GAME_RUNNING) {
        avg_distance1 = avg_origin_distance(&game->road1);
        avg_distance2 = avg_origin_distance(&game->road2);

        signal_horizontal = fmod(road1_a * avg_distance1 + road1_b * avg_distance2, 100) > 50;
        signal_vertical = fmod(road2_a * avg_distance1 + road2_b * avg_distance2, 100) > 50;

        apply_signals(&game->traffic_light, signal_horizontal, signal_vertical);

        fprintf(output_file, "Cicles passed: %u | Signals: %u %u\n", game->stats.cycles_passed, signal_horizontal, signal_vertical);
        wait();
    }
    fclose(output_file);

    return NULL;
}

// Helper functions

double avg_origin_distance(list_t* road) {
    double sum = 0;
    car_t* car;

    for (int i = 0; i < road->size; i++) {
        car = list_get(road, i);

        if (car->pos_x > 0 || car->pos_y > 0) {
            continue;
        }

        sum += sqrt(pow(car->pos_x, 2) + pow(car->pos_y, 2));
    }

    return (double) sum / road->size;;
}

void apply_signals(traffic_light_t* tl, bool horizontal, bool vertical) {
    while (horizontal != tl->h_state) {
        tl_toggle_horizontal(tl);
    }

    while (vertical != tl->v_state) {
        tl_toggle_horizontal(tl);
    }
}

void wait() {
    // Sleep for a short duration to simulate time passing
    struct timespec ts = {0, 500000000}; // 0.5 seconds
    nanosleep(&ts, NULL);
}
