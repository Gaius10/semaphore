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
 * copies (see the config_get_* functions), so they stay valid after config_destroy.
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

struct config_ai_player {
    /** World config */
    /** AI config */
};

struct config_training {
    /** World config (or World generation config) */
    /** AI config */
    /** Training config */
};

typedef enum config_status {
    CONFIG_STATUS_OK,
    /** Memory allocation failed (config_create). */
    CONFIG_STATUS_ENOMEM,
    /** File access error (config_create). */
    CONFIG_STATUS_EACCESS,
    /** File parsing error (config_create). */
    CONFIG_STATUS_EPARSE,
    /** A required key is absent from the file (config_create). */
    CONFIG_STATUS_EMISSING,
} config_status_t;

/**
 * Builds a config from the ini file at `path`. There are no defaults: every key the
 * config needs must be in the file. Required keys are `mode`, plus these when the mode is human:
 * `car_spawn_probability_h`, `car_spawn_probability_v`, `road_length_h`, `road_length_v`,
 * `view_width`, `view_height` and `tick_ms`. The lengths, the view size and the tick must be
 * positive whole numbers.
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
config_view_t config_get_view(const config_t* config);
/** How long a game tick lasts, in milliseconds. Always positive. */
unsigned int config_get_tick_ms(const config_t* config);

#endif
