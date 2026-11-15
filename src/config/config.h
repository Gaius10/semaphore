#ifndef CONFIG_H
#define CONFIG_H

/** Opaque type: the struct is defined in config.c. Use the functions below. */
typedef struct Config config_t;

typedef enum config_mode {
    MODE_HUMAN_PLAYER,
    MODE_AI_PLAYER,
    MODE_TRAINING,
} config_mode_t;

/**
 * Each section of the config is a plain public struct. The config hands them out as
 * copies (see the config_get_* functions), so they stay valid after config_destroy. The one
 * exception is the coefficient arrays of config_ai_player_t.
 */
typedef struct config_human_player {
    /** World config */
    double car_spawn_probability_h;
    double car_spawn_probability_v;
    /**
     * Length of each road, in cars: the most cars that can be on it. Always positive, and at most
     * INT_MAX, since the game uses the lengths as (int) grid coordinates.
     */
    unsigned int road_length_h;
    unsigned int road_length_v;
} config_human_player_t;

/** Size of the terminal view, in cells. Both are always positive. */
typedef struct config_view {
    int width;
    int height;
} config_view_t;

/** Which AI plays in AI mode. */
typedef enum config_agent {
    CONFIG_AGENT_NOOP,               // Never touches a light.
    CONFIG_AGENT_FIXED_TOGGLE,       // Swaps which light is green every `toggle_interval_ticks` ticks.
    CONFIG_AGENT_LINEAR_COMBINATION, // Uses a linear combination of distances to decide which light is green.
} config_agent_t;

/** How the linear combination agent turns its score into a light decision. */
typedef enum binarization_method {
    BINARIZATION_METHOD_THRESHOLD,
} binarization_method_t;

/** AI config. The world it plays in is the same as the human's (see config_human_player_t). */
typedef struct config_ai_player {
    config_agent_t agent;

    /** Ticks a light stays green before the lights swap. Only used by the fixed toggle agent; positive when set. */
    unsigned int toggle_interval_ticks;

    /** Linear combination agent parameters */
    binarization_method_t binarization_method;
    double threshold;

    /**
     * One coefficient per cell of each road: road_length_h and road_length_v of them (see
     * config_human_player_t). Only guaranteed to be set when the agent is linear_combination, and
     * NULL if the file leaves them out. Unlike the rest of this struct, these arrays stay owned by
     * the config and are only valid until config_destroy, so copy what you need.
     */
    double* coeficients_h;
    double* coeficients_v;
} config_ai_player_t;

/**
 * Training config: a genetic algorithm that looks for the best threshold and coefficients of the
 * linear combination agent. It plays the same world as the human (see config_human_player_t) and has
 * no view, so only the world keys and these are needed in training mode.
 *
 * Every number here has already been checked by config_create: the counts are positive, the rates
 * and limits are in range, and elite_count and tournament_size fit in population_size.
 */
typedef struct config_training {
    /** Candidates per generation. At least 2. */
    unsigned int population_size;
    /** How many generations the run lasts. */
    unsigned int generations;
    /** Games played to score one candidate. */
    unsigned int episodes_per_candidate;
    /** Ticks in each of those games. */
    unsigned int episode_ticks;
    /** Fitness of an episode = cars passed - crash_penalty * crashes. 0 or more. */
    double crash_penalty;
    /** Best candidates copied unchanged into the next generation. Less than population_size. */
    unsigned int elite_count;
    /** Candidates drawn at random for each parent selection; the best one wins. At most population_size. */
    unsigned int tournament_size;
    /** Chance that each gene of a child mutates, from 0 to 1. */
    double mutation_rate;
    /** Size of a mutation, as a fraction of the range of the gene. Above 0. */
    double mutation_sigma;
    /** Coefficients are searched in [0, coeficient_max]. Above 0. */
    double coeficient_max;
    /** The threshold is searched in [0, threshold_max]. Above 0. */
    double threshold_max;
    /** Seed of the run: the same seed gives the same run. */
    unsigned int seed;
    /** How many worker processes evaluate candidates at the same time. */
    unsigned int workers;
    /** Folder the results file goes in. Never empty, always shorter than the array. */
    char output_dir[256];
} config_training_t;

typedef enum config_status {
    CONFIG_STATUS_OK,
    CONFIG_STATUS_ENOMEM,   // Memory allocation failed (config_create).
    CONFIG_STATUS_EACCESS,  // File access error (config_create).
    CONFIG_STATUS_EPARSE,   // File parsing error (config_create).
    CONFIG_STATUS_EMISSING, // A required key is absent from the file (config_create).
} config_status_t;

/**
 * Builds a config from the ini file at `path`. There are no defaults: every key the
 * config needs must be in the file. Required keys are `mode`, plus these when the mode is human:
 * `car_spawn_probability_h`, `car_spawn_probability_v`, `road_length_h`, `road_length_v`,
 * `view_width`, `view_height` and `tick_ms`. The lengths, the view size and the tick must be
 * positive whole numbers.
 * When the mode is ai, `agent` (`noop`, `fixed_toggle` or `linear_combination`) is also required.
 * The agent `fixed_toggle` also needs `toggle_interval_ticks` (a positive whole number). The agent
 * `linear_combination` also needs `coeficients_h` and `coeficients_v` (comma-separated numbers,
 * exactly one per cell of that road, so `road_length_h` and `road_length_v` of them) and, with the
 * `threshold` binarization, `threshold`.
 * When the mode is training, the world keys (`car_spawn_probability_h`, `car_spawn_probability_v`,
 * `road_length_h`, `road_length_v`) are required, but not the view keys or `tick_ms`, because
 * training has no view. It also needs every key of config_training_t: `population_size` (at least
 * 2), `generations`, `episodes_per_candidate`, `episode_ticks`, `elite_count` (less than
 * `population_size`), `tournament_size` (at most `population_size`), `seed` and `workers` (positive
 * whole numbers), `crash_penalty` (0 or more), `mutation_rate` (0 to 1), `mutation_sigma`,
 * `coeficient_max` and `threshold_max` (finite numbers above 0) and `training_output_dir` (a
 * non-empty path).
 * Unknown keys are ignored, and key order does not matter.
 *
 * On success, *config holds a new config that must be released with config_destroy.
 * On failure, *config is left untouched and nothing is leaked.
 *
 * Returns CONFIG_STATUS_EACCESS if the file cannot be opened, CONFIG_STATUS_EPARSE if a
 * line is malformed or a value is invalid, and CONFIG_STATUS_EMISSING if a required key
 * is absent.
 */
config_status_t config_create(const char* path, config_t** config);
void config_destroy(config_t** config);

config_mode_t config_get_mode(const config_t* config);
config_human_player_t config_get_human_player(const config_t* config);
config_ai_player_t config_get_ai_player(const config_t* config);
config_training_t config_get_training(const config_t* config);
config_view_t config_get_view(const config_t* config);
/** How long a game tick lasts, in milliseconds. Always positive. */
unsigned int config_get_tick_ms(const config_t* config);

#endif
