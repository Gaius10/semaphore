#include "candidate.h"

#include <stdlib.h>

#include "../agents/ai.h"
#include "../agents/linear_combination.h"
#include "../game/game.h"

typedef struct episode_score {
    unsigned int cars_passed;
    unsigned int crashes;
} episode_score_t;

static candidate_status_t play_episode(
    const config_t* config,
    const double* genome,
    unsigned int seed,
    episode_score_t* score
);

unsigned int candidate_genome_length(const config_t* config) {
    config_human_player_t world = config_get_human_player(config);

    return 1 + world.road_length_h + world.road_length_v;
}

void candidate_genome_ranges(const config_t* config, double* min, double* max) {
    config_training_t training = config_get_training(config);
    unsigned int length = candidate_genome_length(config);

    min[0] = 0.0;
    max[0] = training.threshold_max;
    for (unsigned int g = 1; g < length; g++) {
        min[g] = 0.0;
        max[g] = training.coeficient_max;
    }
}

double candidate_fitness(double cars_passed, double crashes, double crash_penalty) {
    return cars_passed - crash_penalty * crashes;
}

candidate_status_t candidate_play(
    const config_t* config,
    const double* genome,
    unsigned int generation,
    candidate_result_t* result
) {
    if (config == NULL || genome == NULL || result == NULL) {
        return CANDIDATE_STATUS_EINVAL;
    }

    config_training_t training = config_get_training(config);
    double cars_passed = 0.0;
    double crashes = 0.0;

    for (unsigned int episode = 0; episode < training.episodes_per_candidate; episode++) {
        unsigned int seed = training.seed + generation * training.episodes_per_candidate + episode;
        episode_score_t score;

        candidate_status_t status = play_episode(config, genome, seed, &score);
        if (status != CANDIDATE_STATUS_OK) {
            return status;
        }

        cars_passed += score.cars_passed;
        crashes += score.crashes;
    }

    cars_passed /= training.episodes_per_candidate;
    crashes /= training.episodes_per_candidate;

    result->cars_passed = cars_passed;
    result->crashes = crashes;
    result->fitness = candidate_fitness(cars_passed, crashes, training.crash_penalty);
    return CANDIDATE_STATUS_OK;
}

/** One game, in memory: no view, no sleeping. The genome is [threshold, coeficients_h..., coeficients_v...]. */
static candidate_status_t play_episode(
    const config_t* config,
    const double* genome,
    unsigned int seed,
    episode_score_t* score
) {
    config_human_player_t world = config_get_human_player(config);
    config_training_t training = config_get_training(config);

    game_t* game = NULL;
    switch (game_create(config, &game)) {
        case GAME_STATUS_OK:
            break;
        case GAME_STATUS_ENOMEM:
            return CANDIDATE_STATUS_ENOMEM;
        default:
            return CANDIDATE_STATUS_EPLAY;
    }

    const double* coeficients_h = &genome[1];
    const double* coeficients_v = &genome[1 + world.road_length_h];
    ai_t* ai = NULL;
    ai_status_t ai_status = linear_combination_create_from(
        genome[0],
        coeficients_h,
        world.road_length_h,
        coeficients_v,
        world.road_length_v,
        &ai
    );
    if (ai_status != AI_STATUS_OK) {
        game_destroy(&game);
        return ai_status == AI_STATUS_ENOMEM ? CANDIDATE_STATUS_ENOMEM : CANDIDATE_STATUS_EPLAY;
    }

    srand(seed); // From here on the traffic depends only on this seed.
    for (unsigned int tick = 0; tick < training.episode_ticks; tick++) {
        game_exec(game, ai_decide(ai, game));
    }

    score->cars_passed = game_get_cars_passed(game);
    score->crashes = game_get_crashes(game);

    ai_destroy(&ai);
    game_destroy(&game);
    return CANDIDATE_STATUS_OK;
}
