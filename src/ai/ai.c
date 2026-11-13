#include "ai.h"

#include <stddef.h>

#include "agent.h"
#include "fixed_toggle.h"
#include "noop.h"

ai_status_t ai_create(const config_t* config, ai_t** ai) {
    switch (config_get_ai_player(config).agent) {
        case CONFIG_AGENT_NOOP:
            return noop_create(config, ai);
        case CONFIG_AGENT_FIXED_TOGGLE:
            return fixed_toggle_create(config, ai);
    }

    return AI_STATUS_EAGENT;
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
