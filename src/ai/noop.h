#ifndef NOOP_H
#define NOOP_H

#include "ai.h"

/** The NOOP agent: never does anything, it it always answers with the current state of the game. */
ai_status_t noop_create(const config_t* config, ai_t** ai);

#endif
