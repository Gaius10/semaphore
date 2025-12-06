// Caio Corrêa Chaves - 15444406
// Gustavo Henrique Nogueira de Andrade Filho - 16871388
#include "../lib/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>

#include "../lib/game.h"
#include "../lib/ai.h"
#include "../lib/input.h"

struct options {
    enum mode {
        MODE_DEFAULT,
        MODE_PERFORMANCE_STATS
    } mode;

    enum commander {
        COMMANDER_PLAYER,
        COMMANDER_FIXED_TOGGLE,
    } commander;

    unsigned int number_of_games;
};

void*(*commander_factory(enum commander commander_type))(void*);
struct options parse_arguments(int argc, char* argv[]);
void print_usage(FILE* file, char* current_filename);
game_t* run(struct options opts);

/**
 * For readability purposes, one function for each mode
 */
void run_default(struct options opts);
void run_performance_stats(struct options opts);

int main(int argc, char *argv[]) {
    initTermios();

    struct options opts = parse_arguments(argc, argv);

    if (opts.mode == MODE_DEFAULT) {
        run_default(opts);
    }

    if (opts.mode == MODE_PERFORMANCE_STATS) {
        run_performance_stats(opts);
    }

    resetTermios();
    return EXIT_SUCCESS;
}

void run_default(struct options opts) {
    game_t* game = run(opts);
    free(game);
}

void* run_thread(void* arg) {
    struct options opts = *(struct options*)arg;
    return (void*) run(opts);
}

void run_performance_stats(struct options opts) {
    game_t** games = malloc(sizeof(game_t*) * opts.number_of_games);
    pthread_t* threads = malloc(sizeof(pthread_t) * opts.number_of_games);
    FILE* output_file = fopen("performance_stats.txt", "a+");

    for (unsigned int i = 0; i < opts.number_of_games; i++) {
        pthread_create(&threads[i], NULL, run_thread, &opts);
    }

    for (unsigned int i = 0; i < opts.number_of_games; i++) {
        pthread_join(threads[i], (void**) &games[i]);
    }

    fprintf(output_file, "\n=== Performance Statistics ===\n");
    for (unsigned int i = 0; i < opts.number_of_games; i++) {
        game_t* game = games[i];
        char* game_over_reason_strings[] = {
            "N/A",
            "Road Overflow",
            "Collision"
        };

        fprintf(output_file, "Game %u:\n", i + 1);
        fprintf(output_file, "  Cars Created: %u\n", game->stats.cars_created);
        fprintf(output_file, "  Cars Passed: %u\n", game->stats.cars_passed);
        fprintf(output_file, "  Cycles Passed: %u\n", game->stats.cycles_passed);
        fprintf(output_file, "  Average Wait Cycles: %u\n", game->stats.average_wait_cycles);
        fprintf(output_file, "  Game Over Reason: %s\n", game_over_reason_strings[game->stats.game_over_reason]);

        fprintf(output_file, "\n");
    }

    fclose(output_file);
    // Freedom
    for (unsigned int i = 0; i < opts.number_of_games; i++) {
        free(games[i]);
    }
    free(games);
}

game_t* run(struct options opts) {
    if (DEBUG && opts.mode == MODE_DEFAULT) {
        printf("Starting game with the following options:\n");
        printf("Mode: %d\n", opts.mode);
        printf("Commander: %d\n", opts.commander);
        printf("Number of games: %u\n", opts.number_of_games);
        getchar();
    }

    game_t* game = malloc(sizeof(game_t));

    pthread_t car_factory_thread;
    pthread_t car_mover_thread;
    pthread_t world_renderer_thread;
    pthread_t commander_thread;
    pthread_t game_state_thread;

    game_init(game);

    pthread_create(&car_factory_thread, NULL, car_factory, game);
    pthread_create(&car_mover_thread, NULL, car_mover, game);
    pthread_create(&game_state_thread, NULL, game_state_manager, game);
    pthread_create(&commander_thread, NULL, commander_factory(opts.commander), game);

    if (opts.mode == MODE_DEFAULT) {
        pthread_create(&world_renderer_thread, NULL, world_renderer, game);
    }

    pthread_join(car_factory_thread, NULL);
    pthread_join(car_mover_thread, NULL);
    pthread_join(game_state_thread, NULL);

    if (opts.mode == MODE_DEFAULT) {
        pthread_join(world_renderer_thread, NULL);
    }

    pthread_cancel(commander_thread);

    if (DEBUG) {
        printf("Game Over!\n");
    }

    return game;
}

struct options parse_arguments(int argc, char* argv[]) {
    struct options opts = {
        .mode = MODE_DEFAULT,
        .commander = COMMANDER_PLAYER,
        .number_of_games = 1,
    };

    for (uint8_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_usage(stdout, argv[0]);
            resetTermios();
            exit(EXIT_SUCCESS);
        }

        if (strcmp(argv[i], "--mode") == 0) {
            i++;

            if (strcmp(argv[i], "performance_stats") == 0) {
                opts.mode = MODE_PERFORMANCE_STATS;
                continue;
            }

            if (strcmp(argv[i], "default") != 0) {
                fprintf(stderr, "Error: invalid mode: %s\n", argv[i]);
                resetTermios();
                exit(EXIT_FAILURE);
            }

            continue;
        }

        if (strcmp(argv[i], "--commander") == 0) {
            i++;

            if (strcmp(argv[i], "fixed_toggle") == 0) {
                opts.commander = COMMANDER_FIXED_TOGGLE;
                continue;
            }

            if (strcmp(argv[i], "player") != 0) {
                fprintf(stderr, "Error: invalid commander: %s\n", argv[i]);
                resetTermios();
                exit(EXIT_FAILURE);
            }

            continue;
        }

        if (
            strcmp(argv[i], "--number_of_games") == 0 ||
            strcmp(argv[i], "--games") == 0 ||
            strcmp(argv[i], "-n") == 0
        ) {
            i++;

            if (sscanf(argv[i], "%u", &opts.number_of_games) != 1) {
                fprintf(stderr, "Error reading number of games\n");
                resetTermios();
                exit(EXIT_FAILURE);
            }

            continue;
        }

        // If code reaches this point, input is in a invalid format
        fprintf(stderr, "Invalid input format.\n");
        fprintf(stderr, "Use: %s -h for help\n", argv[0]);
        resetTermios();
        exit(EXIT_FAILURE);
    }

    if (opts.number_of_games == 0) {
        fprintf(stderr, "Error: number_of_games must be greater than 0\n");
        print_usage(stderr, argv[0]);
        resetTermios();
        exit(EXIT_FAILURE);
    }

    if (opts.number_of_games > 1 && opts.mode == MODE_DEFAULT) {
        fprintf(stderr, "Error: default mode only works with a single game\n");
        print_usage(stderr, argv[0]);
        resetTermios();
        exit(EXIT_FAILURE);
    }

    if (opts.commander == COMMANDER_PLAYER && opts.mode != MODE_DEFAULT) {
        fprintf(stderr, "Error: player commander only works with default mode\n");
        print_usage(stderr, argv[0]);
        resetTermios();
        exit(EXIT_FAILURE);
    }

    return opts;
}

void*(*commander_factory(enum commander commander_type))(void*) {
    if (commander_type == COMMANDER_PLAYER) {
        return commander;
    }

    if (commander_type == COMMANDER_FIXED_TOGGLE) {
        return ai_commander_fixed_toggle;
    }

    fprintf(stderr, "Error: failed to load commander.\n");
    exit(EXIT_FAILURE);
}

void print_usage(FILE* file, char* current_filename) {
    fprintf(file, "Usage: %s [options]\n", current_filename);
    fprintf(file, "Options:\n");
    fprintf(file, "  -h                      Show this help message and exit\n");
    fprintf(file, "  --mode <mode>           Set the game mode (default: default)\n");
    fprintf(file, "                          Available modes: default, performance_stats\n");
    fprintf(file, "  --commander <commander> Set the commander type (default: player)\n");
    fprintf(file, "                          Available commanders: player, fixed_toggle\n");
    fprintf(file, "  --number_of_games <n>   Set the number of games to play (default: 1)\n");
    fprintf(file, "                          Note: default mode only supports a single game\n");
    fprintf(file, "                          Note: player commander only supports default mode\n");
    fprintf(file, "                          Aliases: --games, -n\n");
}
