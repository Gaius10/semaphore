#ifndef FIXED_TOGGLE_H
#define FIXED_TOGGLE_H

#include "ai.h"

/**
 * The fixed toggle agent: keeps a light green for `toggle_interval_ticks` ticks, then swaps the
 * lights. A swap is a single command that closes the green light and opens the other at the same
 * time, so the lights are never both green. It does not look at the cars.
 */
ai_status_t fixed_toggle_create(const config_t* config, ai_t** ai);

#endif
