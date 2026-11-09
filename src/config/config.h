#ifndef CONFIG_H
#define CONFIG_H

typedef struct config {
    enum {
        MODE_HUMAN_PLAYER,
        MODE_AI_PLAYER,
        MODE_TRAINING,
    } mode;

    struct config_human_player* human_player;
    // struct config_ai_player* ai_player;
    // struct config_training* training; 
} config_t;

struct config_human_player {
    /** World config */
    double car_spawn_probability_h;
    double car_spawn_probability_v;
};

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
    CONFIG_STATUS_EACCESS,
    CONFIG_STATUS_EPARSE,
} config_status_t;

config_status_t config_parse_file(const char* path, config_t* config);

#endif
