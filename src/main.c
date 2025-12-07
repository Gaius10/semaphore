// Caio Corrêa Chaves - 15444406
// Gustavo Henrique Nogueira de Andrade Filho - 16871388

#include "../lib/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../lib/input.h"
#include "../lib/game_runner.h"

struct options parse_arguments(int argc, char* argv[]);
void print_usage(FILE* file, char* current_filename);

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

            if (strcmp(argv[i], "random_toggle") == 0) {
                opts.commander = COMMANDER_RANDOM_TOGGLE;
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

void print_usage(FILE* file, char* current_filename) {
    fprintf(file, "Usage: %s [options]\n", current_filename);
    fprintf(file, "Options:\n");
    fprintf(file, "  -h                      Show this help message and exit\n");
    fprintf(file, "  --mode <mode>           Set the game mode (default: default)\n");
    fprintf(file, "                            Available modes: default, performance_stats\n");
    fprintf(file, "  --commander <commander> Set the commander type (default: player)\n");
    fprintf(file, "                            Available commanders: player, fixed_toggle\n");
    fprintf(file, "  --number_of_games <n>   Set the number of games to play (default: 1)\n");
    fprintf(file, "                            Note: default mode only supports a single game\n");
    fprintf(file, "                            Note: player commander only supports default mode\n");
    fprintf(file, "                            Aliases: --games, -n\n");
}
