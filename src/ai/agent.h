#ifndef AGENT_H
#define AGENT_H

#include "ai.h"

/**
 * The interface every agent implements. Only ai.c and the agents include this: the rest of the
 * game talks to an agent through ai.h.
 *
 * An agent defines its own struct, with an ai_t as its FIRST member, and fills `ops` with its own
 * functions. A pointer to the agent is then also a valid ai_t pointer, and the agent's functions
 * cast it back. See noop.c for the smallest example.
 */
typedef struct agent_ops {
    /** Picks the command for the next tick. Same contract as ai_decide. */
    game_command_t (*decide)(ai_t* ai, const game_t* game);
    /** Frees the agent, whatever its concrete struct is. Never called with NULL. */
    void (*destroy)(ai_t* ai);
} agent_ops_t;

struct Ai {
    const agent_ops_t* ops;
};

#endif
