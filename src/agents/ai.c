#include "ai.h"

#include <stddef.h>

#include "agent.h"
#include "noop.h"
#include "fixed_toggle.h"
#include "linear_combination.h"

ai_status_t ai_create(const config_t* config, ai_t** ai) {
    switch (config_get_ai_player(config).agent) {
        case CONFIG_AGENT_NOOP:
            return noop_create(config, ai);
        case CONFIG_AGENT_FIXED_TOGGLE:
            return fixed_toggle_create(config, ai);
        case CONFIG_AGENT_LINEAR_COMBINATION:
            return linear_combination_create(config, ai);
    }

    return AI_STATUS_EAGENT;
}

game_command_t agent_current_command(const game_t* game) {
    return (game_command_t) {
        .horizontal_light = traffic_light_get_color(game_get_horizontal_light(game)),
        .vertical_light = traffic_light_get_color(game_get_vertical_light(game)),
    };
}

void ai_destroy(ai_t** ai) {
    if (ai == NULL || *ai == NULL) {
        return;
    }

    (*ai)->ops->destroy(*ai);
    *ai = NULL;
}

game_command_t ai_decide(ai_t* ai, const game_t* game) {
    return ai->ops->decide(ai, game);
}
