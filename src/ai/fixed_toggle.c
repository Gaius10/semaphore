#include "fixed_toggle.h"

#include <stdbool.h>
#include <stdlib.h>

#include "agent.h"

typedef struct {
    ai_t base; /* must be first */
    unsigned int interval;
    unsigned int green_ticks;
    game_command_t pending;
} fixed_toggle_t;

static game_command_t fixed_toggle_decide(ai_t* ai, const game_t* game) {
    /**
     * This works because base is the first member of fixed_toggle_t, so we can safely cast ai to
     * fixed_toggle_t* and access its members.
     *
     * In C, the address of a struct is the same as the address of its first member,
     * so this cast is valid and allows us to access the additional members of fixed_toggle_t.
     */
    fixed_toggle_t* self = (fixed_toggle_t*) ai;

    /** Second half of a swap: open the light that was waiting. The new green starts counting now. */
    if (self->pending != GAME_COMMAND_NOOP) {
        game_command_t command = self->pending;
        self->pending = GAME_COMMAND_NOOP;
        self->green_ticks = 0;
        return command;
    }

    bool horizontal_green = traffic_light_get_color(game_get_horizontal_light(game)) == TRAFFIC_LIGHT_GREEN;
    bool vertical_green = traffic_light_get_color(game_get_vertical_light(game)) == TRAFFIC_LIGHT_GREEN;

    /** Same color on both lights is not a state this agent leaves them in: get back to one green right away. */
    if (horizontal_green == vertical_green) {
        self->green_ticks = 0;
        return horizontal_green ? GAME_COMMAND_VERTICAL_TOGGLE : GAME_COMMAND_HORIZONTAL_TOGGLE;
    }

    if (self->green_ticks < self->interval) {
        self->green_ticks++;
        return GAME_COMMAND_NOOP;
    }

    /** First half of a swap: close the green light (the other one is opened next tick). */
    if (horizontal_green) {
        self->pending = GAME_COMMAND_VERTICAL_TOGGLE;
        return GAME_COMMAND_HORIZONTAL_TOGGLE;
    }

    self->pending = GAME_COMMAND_HORIZONTAL_TOGGLE;
    return GAME_COMMAND_VERTICAL_TOGGLE;
}

static void fixed_toggle_destroy(ai_t* ai) {
    free(ai);
}

static const agent_ops_t OPS = {
    .decide = fixed_toggle_decide,
    .destroy = fixed_toggle_destroy,
};

ai_status_t fixed_toggle_create(const config_t* config, ai_t** ai) {
    fixed_toggle_t* self = malloc(sizeof(fixed_toggle_t));
    if (self == NULL) {
        return AI_STATUS_ENOMEM;
    }

    self->base.ops = &OPS;
    self->interval = config_get_ai_player(config).toggle_interval_ticks;
    self->green_ticks = 0;
    self->pending = GAME_COMMAND_NOOP;

    *ai = &self->base;

    return AI_STATUS_OK;
}
