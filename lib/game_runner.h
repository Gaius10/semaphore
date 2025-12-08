#ifndef GAME_RUNNER_H
#define GAME_RUNNER_H

struct options {
    enum mode {
        MODE_DEFAULT,
        MODE_PERFORMANCE_STATS,
        MODE_TRAINING,
    } mode;

    enum commander {
        COMMANDER_PLAYER,
        COMMANDER_FIXED_TOGGLE,
        COMMANDER_RANDOM_TOGGLE,
        COMMANDER_SPECIMEN_MODEL_01,
    } commander;

    unsigned int number_of_games;
};

void run_default(struct options opts);
void run_performance_stats(struct options opts);
void run_training(struct options opts);

#endif
