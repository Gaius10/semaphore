#include "ai.h"

#include <stdlib.h>

struct Ai {
    /** Nothing to remember yet: the only AI is stateless. */
    char unused;
};

ai_status_t ai_create(const config_t* config, ai_t** ai) {
    (void) config;

    ai_t* new_ai = malloc(sizeof(ai_t));
    if (new_ai == NULL) {
        return AI_STATUS_ENOMEM;
    }

    *ai = new_ai;

    return AI_STATUS_OK;
}

void ai_destroy(ai_t** ai) {
    if (ai == NULL || *ai == NULL) {
        return;
    }

    free(*ai);
    *ai = NULL;
}

game_command_t ai_decide(ai_t* ai, const game_t* game) {
    (void) ai;
    (void) game;

    return GAME_COMMAND_NOOP;
}
