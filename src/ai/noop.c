#include "noop.h"

#include <stdlib.h>

#include "agent.h"

typedef struct {
    ai_t base; /* must be first */
    /* Nothing to remember: this agent is stateless. */
} noop_t;

static game_command_t noop_decide(ai_t* ai, const game_t* game) {
    (void) ai;
    (void) game;

    return GAME_COMMAND_NOOP;
}

static void noop_destroy(ai_t* ai) {
    free(ai);
}

static const agent_ops_t OPS = {
    .decide = noop_decide,
    .destroy = noop_destroy,
};

ai_status_t noop_create(const config_t* config, ai_t** ai) {
    (void) config;

    noop_t* noop = malloc(sizeof(noop_t));
    if (noop == NULL) {
        return AI_STATUS_ENOMEM;
    }

    noop->base.ops = &OPS;
    *ai = &noop->base;

    return AI_STATUS_OK;
}
