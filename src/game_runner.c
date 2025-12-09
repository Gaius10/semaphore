#include "../lib/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "../lib/game.h"
#include "../lib/game_runner.h"
#include "../lib/ai_commanders.h"
#include "../lib/input.h"

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
void population_test(double* population, unsigned len, double* champion);

// public
void run_training(struct options opts) {
    srand((unsigned) time(NULL));
    FILE* champions_file = fopen("champions.csv", "r");
    fseek(champions_file, 0L, SEEK_END);
    long int fpos = ftell(champions_file);
    fseek(champions_file, 0L, SEEK_SET);
    double* champion = NULL;
    double* new_champion = malloc(sizeof(double) * 7);
    unsigned int last_champion_id = 0;
    double* population = malloc(sizeof(double) * 6 * TRAINING_POPULATION_LEN);
    char line[1024];
    char last_line[1024];

    if (population == NULL) {
        fprintf(stderr, "Error allocating memmory for population.\n");
        fclose(champions_file);
        resetTermios();
        exit(EXIT_FAILURE);
    }

    if (fpos > 55) {
        champion = malloc(sizeof(double) * 7);
        if (champion == NULL) {
            fprintf(stderr, "Error allocating memmory for champion.\n");
            fclose(champions_file);
            resetTermios();
            exit(EXIT_FAILURE);
        }

        while (fgets(line, 1024, champions_file) != NULL) {
            if (line[0] == '\n' || line[0] == '\0') continue;
            strncpy(last_line, line, 1023);
            last_line[1023] = '\0';
        }

        if (
            sscanf(
                last_line,
                "%u,%lf,%lf,%lf,%lf,%lf,%lf,%lf\n",
                &last_champion_id,
                &champion[0],
                &champion[1],
                &champion[2],
                &champion[3],
                &champion[4],
                &champion[5],
                &champion[6]
            ) != 8
        ) {
            fprintf(stderr, "Error reading champion data.\n");
            fclose(champions_file);
            resetTermios();
            exit(EXIT_FAILURE);
        }

        printf(
            "Read champion: %u %lf %lf %lf %lf %lf %lf %lf\n",
            last_champion_id++,
            champion[0],
            champion[1],
            champion[2],
            champion[3],
            champion[4],
            champion[5],
            champion[6]
        );
        getchar();
    } else {
        printf("No previous champion.\n");
        getchar();
    }
    fclose(champions_file);

    for (unsigned i = 0; i < TRAINING_MAX_GENERATIONS; i++) {
        population_init(population, TRAINING_POPULATION_LEN, champion);

        if (champion == NULL) {
            champion = malloc(sizeof(double) * 7);

            if (champion == NULL) {
                fprintf(stderr, "Error allocating memmory for champion data.\n");
                exit(EXIT_FAILURE);
            }

            champion[0] = 0.0f;
            champion[1] = 0.0f;
            champion[2] = 0.0f;
            champion[3] = 0.0f;
            champion[4] = 0.0f;
            champion[5] = 0.0f;
            champion[6] = 0.0f;
        }

        new_champion[0] = 0.0f;
        new_champion[1] = 0.0f;
        new_champion[2] = 0.0f;
        new_champion[3] = 0.0f;
        new_champion[4] = 0.0f;
        new_champion[5] = 0.0f;
        new_champion[6] = 0.0f;

        population_test(population, TRAINING_POPULATION_LEN, new_champion);

        if (new_champion[6] > champion[6]) {
            champions_file = fopen("champions.csv", "a+");
            fprintf(
                champions_file,
                "%u,%.18lf,%.18lf,%.18lf,%.18lf,%.18lf,%.18lf,%.18lf\n",
                last_champion_id++,
                champion[0],
                champion[1],
                champion[2],
                champion[3],
                champion[4],
                champion[5],
                champion[6]
            );
            fclose(champions_file);

            champion[0] = new_champion[0];
            champion[1] = new_champion[1];
            champion[2] = new_champion[2];
            champion[3] = new_champion[3];
            champion[4] = new_champion[4];
            champion[5] = new_champion[5];
            champion[6] = new_champion[6];
        }
    }
}

void random_population(double* population, unsigned len);
void from_champion(double* population, unsigned len, double* champion);

void population_init(double* population, unsigned len, double* champion) {
    if (champion == NULL) {
        random_population(population, len);
        return;
    }

    from_champion(population, len, champion);
}

double rand_from(double min, double max)
{
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

void random_population(double* population, unsigned len) {
    for (unsigned i = 0; i < len; i++) {
        population[i*6 + 0] = rand_from(-1000, 1000);
        population[i*6 + 1] = rand_from(-1000, 1000);
        population[i*6 + 2] = rand_from(-1000, 1000);
        population[i*6 + 3] = rand_from(-1000, 1000);
        population[i*6 + 4] = rand_from(-1000, 1000);
        population[i*6 + 5] = rand_from(-1000, 1000);
    }
}

void from_champion(double* population, unsigned len, double* champion) {
    for (unsigned i = 0; i < len; i++) {
        population[i*6 + 0] = champion[0] + rand_from(-100, 100);
        population[i*6 + 1] = champion[1] + rand_from(-100, 100);
        population[i*6 + 2] = champion[2] + rand_from(-100, 100);
        population[i*6 + 3] = champion[3] + rand_from(-100, 100);
        population[i*6 + 4] = champion[4] + rand_from(-100, 100);
        population[i*6 + 5] = champion[5] + rand_from(-100, 100);
    }
}

void population_test(double* population, unsigned len, double* champion) {
    struct options opts = {
        .mode = MODE_PERFORMANCE_STATS,
        .commander = COMMANDER_SPECIMEN_MODEL_01,
        .number_of_games = 5,
    };

    double champion_avg_score = *(champion + 4);
    unsigned scores[5];
    double avg_score = 0;
    FILE* model_file;
    FILE* performance_file;

    for (unsigned i = 0; i < len; i++) {
        model_file = fopen("model.cfg", "w");
        fprintf(
            model_file,
            "%lf %lf %lf %lf %lf %lf",
            population[i*6 + 0],
            population[i*6 + 1],
            population[i*6 + 2],
            population[i*6 + 3],
            population[i*6 + 4],
            population[i*6 + 5]
        );
        fclose(model_file);

        run_performance_stats(opts);

        performance_file = fopen("performance_stats.csv", "r");
        // game_id,cars_created,cars_passed,average_wait_cycles,cycles_passed,final_status,game_over_reason
        if (fscanf(performance_file, "%*s,%*s,%*s,%*s,%*s,%*s,%*s\n") != 0) {
            fprintf(stderr, "Error skipping first line of performance file.\n");
            fclose(performance_file);
            exit(EXIT_FAILURE);
        }

        for (unsigned i = 0; i < opts.number_of_games; i++) {
            if (
                fscanf(
                    performance_file,
                    "%*u,%*u,%u,%*u,%*u,%*[^\n]s,%*[^\n]s\n",
                    &scores[i]
                ) != 1
            ) {
                fprintf(stderr, "Error reading performance.\n");
                fclose(performance_file);
                exit(EXIT_FAILURE);
            }
        }
        fclose(performance_file);

        unsigned sum = 0;
        for (unsigned i = 0; i < opts.number_of_games; i++) {
            sum += scores[i];
        }

        avg_score = (double) sum / opts.number_of_games;
        if (avg_score > champion_avg_score) {
            champion_avg_score = avg_score;
            champion[0] = population[i*6 + 0];
            champion[1] = population[i*6 + 1];
            champion[2] = population[i*6 + 2];
            champion[3] = population[i*6 + 3];
            champion[4] = population[i*6 + 4];
            champion[5] = population[i*6 + 5];
            champion[6] = champion_avg_score;
        }
    }
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

    fclose(output_file);
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
