#include "agent.h"
#include "fixed_toggle.h"

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    ai_t base; /* must be first */
    unsigned int interval;
    unsigned int green_ticks;
} fixed_toggle_t;

static void fixed_toggle_destroy(ai_t* ai);
static game_command_t fixed_toggle_decide(ai_t* ai, const game_t* game);
static traffic_light_color_t opposite(traffic_light_color_t color);
const agent_ops_t fixed_toggle_ops = {
    .decide = fixed_toggle_decide,
    .destroy = fixed_toggle_destroy,
};

ai_status_t fixed_toggle_create(const config_t* config, ai_t** ai) {
    fixed_toggle_t* self = malloc(sizeof(fixed_toggle_t));
    if (self == NULL) {
        return AI_STATUS_ENOMEM;
    }

    self->base.ops = &fixed_toggle_ops;
    self->interval = config_get_ai_player(config).toggle_interval_ticks;
    self->green_ticks = 0;
    
    *ai = &self->base;

    return AI_STATUS_OK;
}

static void fixed_toggle_destroy(ai_t* ai) {
    free(ai);
}

static game_command_t fixed_toggle_decide(ai_t* ai, const game_t* game) {
    /**
     * This works because base is the first member of fixed_toggle_t, so we can safely cast ai to
     * fixed_toggle_t* and access its members.
     *
     * In C, the address of a struct is the same as the address of its first member,
     * so this cast is valid and allows us to access the additional members of fixed_toggle_t.
     */
    fixed_toggle_t* self = (fixed_toggle_t*) ai;

    game_command_t current = agent_current_command(game);
    if (self->green_ticks < self->interval) {
        self->green_ticks++;
        return current;
    }

    /** The green light has had its time: swap the colors of the two lights. The new green starts counting now. */
    self->green_ticks = 0;
    return (game_command_t) {
        .horizontal_light = opposite(current.horizontal_light),
        .vertical_light = opposite(current.vertical_light),
    };
}

static traffic_light_color_t opposite(traffic_light_color_t color) {
    return color == TRAFFIC_LIGHT_GREEN ? TRAFFIC_LIGHT_RED : TRAFFIC_LIGHT_GREEN;
}
