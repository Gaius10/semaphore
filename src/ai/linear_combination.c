#include "agent.h"
#include "linear_combination.h"

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    ai_t base; /* must be first */
    binarization_method_t binarization_method;
    double threshold;
} linear_combination_t;

static void linear_combination_destroy(ai_t* ai);
static game_command_t linear_combination_decide(ai_t* ai, const game_t* game);
const agent_ops_t linear_combination_ops = {
    .decide = linear_combination_decide,
    .destroy = linear_combination_destroy,
};

ai_status_t linear_combination_create(const config_t* config, ai_t** ai) {
    linear_combination_t* self = malloc(sizeof(linear_combination_t));
    if (!self) {
        return AI_STATUS_ENOMEM;
    }

    self->base.ops = &linear_combination_ops;
    self->binarization_method = config_get_ai_player(config).binarization_method;
    self->threshold = config_get_ai_player(config).threshold;

    *ai = (ai_t*)self;
    return AI_STATUS_OK;
}

static void linear_combination_destroy(ai_t* ai) {
    free(ai);
}

static game_command_t linear_combination_decide(ai_t* ai, const game_t* game) {
    linear_combination_t* self = (linear_combination_t*)ai;

    /**
     * @todo
     *
     * This implementation will demand a new system of commands.
     * The system will be able to change the entire controllable
     * state of the game at once, not just the traffic lights.
     * 
     * e.g. each command will be a new state of the game, not a 
     * single change.
     */
}
