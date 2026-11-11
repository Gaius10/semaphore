#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "config/config.h"
#include "game/game.h"
#include "game/view.h"
#include "input/input.h"

/** State shared by the game thread and the input thread. */
typedef struct shared {
    pthread_mutex_t lock;
    game_command_t pending_command;
    bool quit;
} shared_t;

typedef struct game_thread_args {
    shared_t* shared;
    game_t* game;
    view_t* view;
} game_thread_args_t;

typedef struct input_thread_args {
    shared_t* shared;
    input_t* input;
} input_thread_args_t;

static int run(const config_t* config);
static int mode_run_human_player(const config_t* config);
static void* game_thread_main(void* arg);
static void* input_thread_main(void* arg);
static bool shared_should_quit(shared_t* shared);
static void shared_quit(shared_t* shared);

int main(int argc, char** argv) {
    srand((unsigned int) time(NULL));

    config_t* config = NULL;
    config_status_t _config = config_create("config.ini", &config);
    if (_config == CONFIG_STATUS_ENOMEM) {
        printf("Error creating config: out of memory.\n");
        return 1;
    }

    if (_config == CONFIG_STATUS_EACCESS) {
        printf("Error accessing config file.\n");
        printf("Make sure the file 'config.ini' exists and is readable.\n");
        return 1;
    }

    if (_config == CONFIG_STATUS_EPARSE) {
        printf("Error parsing config file.\n");
        printf("Check the format of the config file.\n");
        return 1;
    }

    if (_config == CONFIG_STATUS_EMISSING) {
        printf("Missing a required key in the config file.\n");
        printf("Check that 'mode' and the settings for that mode are all set.\n");
        return 1;
    }

    int exit_code = run(config);

    config_destroy(&config);
    return exit_code;
}

static int run(const config_t* config) {
    if (config_get_mode(config) != MODE_HUMAN_PLAYER) {
        printf("Not implemented yet.\n");
        printf("Only human player mode is currently supported.\n");
        return 1;
    }

    int _run = mode_run_human_player(config);
    if (_run != 0) {
        printf("Error running game.\n");
        return 1;
    }

    return 0;
}

static int mode_run_human_player(const config_t* config) {
    game_t* game = NULL;

    game_status_t _game = game_create(config, &game);
    if (_game == GAME_STATUS_ENOMEM) {
        printf("Error creating game: out of memory.\n");
        return 1;
    }

    config_view_t view_config = config_get_view(config);
    view_t* view = NULL;
    view_status_t _view = view_create(stdout, view_config.width, view_config.height, &view);
    if (_view != VIEW_STATUS_OK) {
        printf("Error creating view.\n");
        game_destroy(&game);
        return 1;
    }

    input_t* input = NULL;
    if (input_create(stdin, &input) != INPUT_STATUS_OK) {
        printf("Error creating input.\n");
        view_destroy(&view);
        game_destroy(&game);
        return 1;
    }

    shared_t shared = { .pending_command = GAME_COMMAND_NOOP, .quit = false };
    pthread_mutex_init(&shared.lock, NULL);

    game_thread_args_t game_args = { .shared = &shared, .game = game, .view = view };
    input_thread_args_t input_args = { .shared = &shared, .input = input };

    pthread_t game_thread;
    pthread_t input_thread;
    pthread_create(&game_thread, NULL, game_thread_main, &game_args);
    pthread_create(&input_thread, NULL, input_thread_main, &input_args);
    pthread_join(game_thread, NULL);
    pthread_join(input_thread, NULL);

    pthread_mutex_destroy(&shared.lock);
    input_destroy(&input);
    view_destroy(&view);
    game_destroy(&game);
    return 0;
}

/** Ticks the game once per second until the game ends or the player quits. */
static void* game_thread_main(void* arg) {
    game_thread_args_t* args = arg;

    while (!shared_should_quit(args->shared)) {
        pthread_mutex_lock(&args->shared->lock);
        game_command_t command = args->shared->pending_command;
        args->shared->pending_command = GAME_COMMAND_NOOP;
        pthread_mutex_unlock(&args->shared->lock);

        game_state_t state = game_exec(args->game, command);
        view_render(
            args->view,
            game_get_horizontal_road(args->game),
            game_get_vertical_road(args->game),
            game_get_horizontal_light(args->game),
            game_get_vertical_light(args->game)
        );

        if (state == GAME_STATE_ENDED) {
            break;
        }

        sleep(1);
    }

    // Tells the input thread to stop too.
    shared_quit(args->shared);
    return NULL;
}

/** Turns key presses into commands for the game, until the player quits or the game thread stops. */
static void* input_thread_main(void* arg) {
    input_thread_args_t* args = arg;

    while (!shared_should_quit(args->shared)) {
        input_action_t action = input_read(args->input);

        if (action == INPUT_ACTION_QUIT) {
            shared_quit(args->shared);
        } else if (action == INPUT_ACTION_HORIZONTAL_TOGGLE || action == INPUT_ACTION_VERTICAL_TOGGLE) {
            pthread_mutex_lock(&args->shared->lock);
            args->shared->pending_command = action == INPUT_ACTION_HORIZONTAL_TOGGLE
                ? GAME_COMMAND_HORIZONTAL_TOGGLE
                : GAME_COMMAND_VERTICAL_TOGGLE;
            pthread_mutex_unlock(&args->shared->lock);
        }
    }

    return NULL;
}

static bool shared_should_quit(shared_t* shared) {
    pthread_mutex_lock(&shared->lock);
    bool quit = shared->quit;
    pthread_mutex_unlock(&shared->lock);
    return quit;
}

static void shared_quit(shared_t* shared) {
    pthread_mutex_lock(&shared->lock);
    shared->quit = true;
    pthread_mutex_unlock(&shared->lock);
}
