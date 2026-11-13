#ifndef FIXED_TOGGLE_H
#define FIXED_TOGGLE_H

#include "ai.h"

/**
 * The fixed toggle agent: keeps a light green for `toggle_interval_ticks` ticks, then swaps the
 * lights. It closes the green one and, on the next tick, opens the other, so there is one all-red
 * tick in between and the lights are never both green. It does not look at the cars.
 *
 * If both lights show the same color (the game started that way, or someone else moved them), it
 * fixes that on its first tick, leaving the horizontal light green.
 */
ai_status_t fixed_toggle_create(const config_t* config, ai_t** ai);

#endif
