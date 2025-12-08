#include "../lib/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#include "../lib/game.h"
#include "../lib/game_runner.h"
#include "../lib/ai_commanders.h"

typedef struct gamebag {
    game_t game;
    pthread_t car_factory_thread;
    pthread_t car_mover_thread;
    pthread_t world_renderer_thread;
    pthread_t commander_thread;
    pthread_t state_manager_thread;
} gamebag_t;

typedef struct observer_args {
    gamebag_t* gamebags;
    struct options opts;
} observer_args_t;

void gamebag_init(gamebag_t* gamebag, struct options opts);
void gamebag_join(gamebag_t* gamebag);

// public
void run_default(struct options opts) {
    if (DEBUG) {
        printf("Starting game with the following options:\n");
        printf("Mode: %d\n", opts.mode);
        printf("Commander: %d\n", opts.commander);
        printf("Number of games: %u\n", opts.number_of_games);
        getchar();
    }

    gamebag_t* gamebag = malloc(sizeof(gamebag_t));

    gamebag_init(gamebag, opts);
    gamebag_join(gamebag);

    free(gamebag);
}

void population_init(double* population, unsigned len, double* champion);

// public
void run_training(struct options opts) {
    FILE* champions_file = fopen("training/champions.csv", "a+");

    fseek(champions_file, 0L, SEEK_END);
    long int fpos = ftell(champions_file);
    double* champion = NULL;
    double* population = malloc(sizeof(double) * 4 * TRAINING_POPULATION_LEN);

    if (population == NULL) {
        fprintf(stderr, "Error allocating memmory for population.\n");
        exit(EXIT_FAILURE);
    }

    // No champion registered
    if (fpos > 55) {
        // goes to start of last line
        while (fgetc(champions_file) != '\n') {
            fseek(champions_file, --fpos, SEEK_SET);
        }

        champion = malloc(sizeof(double) * 4);
        if (
            champion == NULL ||
            fscanf(
                champions_file,
                "%*u,%lf,%lf,%lf,%lf",
                champion + 0,
                champion + 1,
                champion + 2,
                champion + 3
            ) != 4
        ) {
            fprintf(stderr, "Error reading last champion.\n");
            exit(EXIT_FAILURE);
        }

        printf(
            "Read champion: %lf %lf %lf %lf\n",
            *(champion + 0),
            *(champion + 1),
            *(champion + 2),
            *(champion + 3)
        );
    } else {
        printf("No champion.\n");
    }


    fclose(champions_file);
    // Initialize population
    // * Test each specimen
    // Save the better
    // Kill everyone
    // Mutate champion (generate new population)
    // goto *
}

void* performance_observer(void* args);
uint8_t count_running_games(gamebag_t* gamebags, struct options opts);

// public
void run_performance_stats(struct options opts) {
    FILE* output_file = fopen("performance_stats.csv", "w");

    gamebag_t* gamebags = malloc(sizeof(gamebag_t) * opts.number_of_games);
    for (unsigned i = 0; i < opts.number_of_games; i++) {
        game_init(&gamebags[i].game);
    }

    pthread_t observer_thread;
    observer_args_t observer_args = {
        .gamebags = gamebags,
        .opts = opts
    };

    // Start games
    pthread_create(&observer_thread, NULL, performance_observer, &observer_args);
    for (unsigned i = 0; i < opts.number_of_games; i++) {
        gamebag_init(&gamebags[i], opts);

        // This would limit number of running games at a time
        // while (count_running_games(gamebags, opts) >= 3) {
        //     sched_yield();
        // }
    }

    // Syncronize threads
    for (unsigned i = 0; i < opts.number_of_games; i++) {
        gamebag_join(&gamebags[i]);
    }
    pthread_join(observer_thread, NULL);

    // Output stats
    fprintf(output_file, "game_id,cars_created,cars_passed,average_wait_cycles,cycles_passed,final_status,game_over_reason\n");
    char* reason_phrase[] = {
        "N/A",
        "Road Overflow",
        "Collision"
    };

    char* status_phrase[] = {
        "Waiting",
        "Running",
        "Game Over"
    };

    for (unsigned i = 0; i < opts.number_of_games; i++) {
        fprintf(output_file, "%u,%u,%u,%u,%u,%s,%s\n",
            i + 1,
            gamebags[i].game.stats.cars_created,
            gamebags[i].game.stats.cars_passed,
            gamebags[i].game.stats.average_wait_cycles,
            gamebags[i].game.stats.cycles_passed,
            status_phrase[gamebags[i].game.status],
            reason_phrase[gamebags[i].game.stats.game_over_reason]
        );
    }
}

void* performance_observer(void* args) {
    gamebag_t* gamebags = ((observer_args_t*)args)->gamebags;
    struct options opts = ((observer_args_t*)args)->opts;

    bool some_game_running;
    char* reason_phrase[] = {
        "N/A",
        "Road Overflow",
        "Collision"
    };

    char* status_phrase[] = {
        "Waiting",
        "Running",
        "Game Over"
    };

    do {
        nanosleep(&(struct timespec){0, 1000 * 1000 * 100}, NULL);
        system("clear");

        some_game_running = false;

        for (unsigned i = 0; i < opts.number_of_games; i++) {
            if (gamebags[i].game.status == GAME_RUNNING) {
                some_game_running = true;
            }

            printf("Game %u: Status: %s | Cars Created: %u | Cars Passed: %u | Average Wait Cycles: %u | Cycles Passed: %u | Game Over Reason: %s\n",
                i + 1,
                status_phrase[gamebags[i].game.status],
                gamebags[i].game.stats.cars_created,
                gamebags[i].game.stats.cars_passed,
                gamebags[i].game.stats.average_wait_cycles,
                gamebags[i].game.stats.cycles_passed,
                reason_phrase[gamebags[i].game.stats.game_over_reason]
            );
        }
    } while (some_game_running);

    return NULL;
}

uint8_t count_running_games(gamebag_t* gamebags, struct options opts) {
    uint8_t count = 0;

    for (unsigned i = 0; i < opts.number_of_games; i++) {
        if (gamebags[i].game.status == GAME_RUNNING) {
            count++;
        }
    }

    return count;
}

void*(*renderer_factory(enum mode game_mode))(void*);
void*(*commander_factory(enum commander commander_type))(void*);

void gamebag_init(gamebag_t* gamebag, struct options opts) {
    game_init(&gamebag->game);

    pthread_create(&gamebag->car_factory_thread,    NULL, car_factory,                       &gamebag->game);
    pthread_create(&gamebag->car_mover_thread,      NULL, car_mover,                         &gamebag->game);
    pthread_create(&gamebag->state_manager_thread,  NULL, game_state_manager,                &gamebag->game);
    pthread_create(&gamebag->world_renderer_thread, NULL, renderer_factory(opts.mode),       &gamebag->game);
    pthread_create(&gamebag->commander_thread,      NULL, commander_factory(opts.commander), &gamebag->game);
}

void gamebag_join(gamebag_t* gamebag) {
    pthread_join(gamebag->car_factory_thread,    NULL);
    pthread_join(gamebag->car_mover_thread,      NULL);
    pthread_join(gamebag->state_manager_thread,  NULL);
    pthread_join(gamebag->world_renderer_thread, NULL);

    pthread_cancel(gamebag->commander_thread);
}

void*(*commander_factory(enum commander commander_type))(void*) {
    if (commander_type == COMMANDER_FIXED_TOGGLE) {
        return ai_commander_fixed_toggle;
    }

    if (commander_type == COMMANDER_RANDOM_TOGGLE) {
        return ai_commander_random_toggle;
    }

    if (commander_type == COMMANDER_SPECIMEN_MODEL_01) {
        return ai_commander_specimen_model_01;
    }

    return commander;
}

void*(*renderer_factory(enum mode game_mode))(void*) {
    if (game_mode == MODE_PERFORMANCE_STATS) {
        return world_renderer_mock;
    }

    return world_renderer;
}
