#ifndef MODE_TRAINING_H
#define MODE_TRAINING_H

#include "../config/config.h"

/**
 * Runs training mode: a genetic algorithm looks for the threshold and the coefficients of the linear
 * combination agent that pass the most cars. Candidates are scored by playing real games without a
 * view (see candidate.h), and every generation's champion is written to a new CSV file in the
 * config's `training_output_dir` (see results_csv.h), created if it is missing. The file is flushed
 * after every generation, so stopping with Ctrl-C leaves everything done so far.
 *
 * Prints one line per generation and, at the end, the best genome as ready-to-paste lines for the
 * `[ai_player]` section of config.ini, and the path of the CSV.
 *
 * Blocks until the last generation is done.
 *
 * @return 0 on success, non-zero if the run could not be set up or was cut short by an error.
 */
int mode_run_training(const config_t* config);

#endif
