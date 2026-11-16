#define _POSIX_C_SOURCE 200809L /* sigaction is hidden under strict -std=c2x */

#include "ai_player.h"

#include <signal.h>
#include <stdio.h>

#include "../agents/ai.h"
#include "../game/game.h"
#include "../game/view.h"
#include "../recording/recorder.h"
#include "sleep.h"

static volatile sig_atomic_t interrupted = 0;

static void on_interrupt(int signal_number) {
    (void) signal_number;
    interrupted = 1;
}

int mode_run_ai_player(const config_t* config) {
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

    ai_t* ai = NULL;
    if (ai_create(config, &ai) != AI_STATUS_OK) {
        printf("Error creating AI.\n");
        view_destroy(&view);
        game_destroy(&game);
        return 1;
    }

    recorder_t* recorder = NULL;
    char record_path[512];
    const char* record_dir = config_get_record_dir(config);
    if (record_dir[0] != '\0') {
        if (recorder_open_in(record_dir, config, record_path, sizeof(record_path), &recorder) != RECORDER_STATUS_OK) {
            printf("Error creating a recording in '%s'.\n", record_dir);
            ai_destroy(&ai);
            view_destroy(&view);
            game_destroy(&game);
            return 1;
        }

        recorder_write_tick(recorder, 0, game);
    }

    // Ctrl-C stops the loop, so the game is cleaned up and the recording path is printed
    struct sigaction handler = { .sa_handler = on_interrupt };
    struct sigaction previous;
    sigemptyset(&handler.sa_mask);
    interrupted = 0;
    sigaction(SIGINT, &handler, &previous);

    unsigned int tick_ms = config_get_tick_ms(config);
    unsigned int tick = 0;
    game_state_t state = GAME_STATE_RUNNING;

    while (state != GAME_STATE_ENDED && !interrupted) {
        game_command_t command = ai_decide(ai, game);
        state = game_exec(game, command);
        tick++;

        if (recorder != NULL) {
            recorder_write_tick(recorder, tick, game);
        }

        view_render(
            view,
            game_get_horizontal_road(game),
            game_get_vertical_road(game),
            game_get_horizontal_light(game),
            game_get_vertical_light(game)
        );

        if (state != GAME_STATE_ENDED && !interrupted) {
            sleep_ms(tick_ms);
        }
    }

    sigaction(SIGINT, &previous, NULL);

    if (recorder != NULL) {
        recorder_close(&recorder);
        printf("Recorded %u ticks to %s\n", tick, record_path);
    }

    ai_destroy(&ai);
    view_destroy(&view);
    game_destroy(&game);
    return 0;
}
