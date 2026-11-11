#include "ai_player.h"

#include <stdio.h>

#include "../ai/ai.h"
#include "../game/game.h"
#include "../game/view.h"
#include "sleep.h"

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

    unsigned int tick_ms = config_get_tick_ms(config);
    game_state_t state = GAME_STATE_RUNNING;

    while (state != GAME_STATE_ENDED) {
        game_command_t command = ai_decide(ai, game);
        state = game_exec(game, command);
        view_render(
            view,
            game_get_horizontal_road(game),
            game_get_vertical_road(game),
            game_get_horizontal_light(game),
            game_get_vertical_light(game)
        );

        if (state != GAME_STATE_ENDED) {
            sleep_ms(tick_ms);
        }
    }

    ai_destroy(&ai);
    view_destroy(&view);
    game_destroy(&game);
    return 0;
}
