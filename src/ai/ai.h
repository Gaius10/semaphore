#ifndef AI_H
#define AI_H

#include "../config/config.h"
#include "../game/game.h"

/**
 * The AI that plays the game in AI mode. It only looks at the game and answers with the command
 * it wants to run on the next tick, so the mode decides when to ask and what to do with the answer.
 *
 * For now there is a single AI, and it never does anything (it always answers
 * GAME_COMMAND_NOOP). It exists to exercise the architecture around it.
 */
typedef struct Ai ai_t;

typedef enum ai_status {
    /** The operation completed successfully. */
    AI_STATUS_OK,
    /** Memory allocation failed (ai_create). */
    AI_STATUS_ENOMEM,
} ai_status_t;

/**
 * On success, *ai holds a new AI that must be released with ai_destroy.
 * The AI copies what it needs from the config, so the config may be destroyed right away.
 */
ai_status_t ai_create(const config_t* config, ai_t** ai);
void ai_destroy(ai_t** ai);

/**
 * Picks the command for the next tick, given the game as it is now. The game is read-only here:
 * the AI never runs the tick itself.
 */
game_command_t ai_decide(ai_t* ai, const game_t* game);

#endif
