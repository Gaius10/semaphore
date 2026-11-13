#ifndef NOOP_H
#define NOOP_H

#include "ai.h"

/** The NOOP agent: never does anything, it always answers GAME_COMMAND_NOOP. */
ai_status_t noop_create(const config_t* config, ai_t** ai);

#endif
