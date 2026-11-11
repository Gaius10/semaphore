#ifndef MODE_AI_PLAYER_H
#define MODE_AI_PLAYER_H

#include "../config/config.h"

/**
 * Runs the game in AI player mode: every tick the AI picks a command, the game runs the tick and
 * the view draws it. There is no input thread: stop it with Ctrl-C.
 *
 * Blocks until the game ends.
 *
 * @return 0 on success, non-zero if the game could not be set up.
 */
int mode_run_ai_player(const config_t* config);

#endif
