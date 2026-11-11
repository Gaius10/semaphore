#ifndef MODE_HUMAN_PLAYER_H
#define MODE_HUMAN_PLAYER_H

#include "../config/config.h"

/**
 * Runs the game in human player mode: a game thread ticks and renders the game
 * while an input thread reads the player's commands.
 *
 * Blocks until the game ends or the player quits.
 *
 * @return 0 on success, non-zero if the game could not be set up.
 */
int mode_run_human_player(const config_t* config);

#endif
