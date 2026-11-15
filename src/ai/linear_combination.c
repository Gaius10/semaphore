#include "agent.h"
#include "linear_combination.h"

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    ai_t base; /* must be first */
    binarization_method_t binarization_method;
    double threshold;
    double* coeficients_h;
    double* coeficients_v;
} linear_combination_t;

static void linear_combination_destroy(ai_t* ai);
static game_command_t linear_combination_decide(ai_t* ai, const game_t* game);
const agent_ops_t linear_combination_ops = {
    .decide = linear_combination_decide,
    .destroy = linear_combination_destroy,
};

ai_status_t linear_combination_create(const config_t* config, ai_t** ai) {
    linear_combination_t* self = malloc(sizeof(linear_combination_t));
    if (!self) {
        return AI_STATUS_ENOMEM;
    }

    config_ai_player_t ai_config = config_get_ai_player(config);

    self->base.ops = &linear_combination_ops;
    self->binarization_method = ai_config.binarization_method;
    self->threshold = ai_config.threshold;
    self->coeficients_h = ai_config.coeficients_h;
    self->coeficients_v = ai_config.coeficients_v;

    *ai = (ai_t*)self;
    return AI_STATUS_OK;
}

static void linear_combination_destroy(ai_t* ai) {
    free(ai);
}

static double combination(const road_t* road, const traffic_light_t* light, const double* coeficients);
static bool signal(const double combination, double threshold);
static game_command_t linear_combination_decide(ai_t* ai, const game_t* game) {
    linear_combination_t* self = (linear_combination_t*) ai;

    const road_t* h_road = game_get_horizontal_road(game);
    const road_t* v_road = game_get_vertical_road(game);

    const traffic_light_t* h_light = game_get_horizontal_light(game);
    const traffic_light_t* v_light = game_get_vertical_light(game);

    double h_combination = combination(h_road, h_light, self->coeficients_h);
    double v_combination = combination(v_road, v_light, self->coeficients_v);

    bool h_signal = signal(h_combination, self->threshold);
    bool v_signal = signal(v_combination, self->threshold);

    return (game_command_t) {
        .horizontal_light = h_signal ? TRAFFIC_LIGHT_GREEN : TRAFFIC_LIGHT_RED,
        .vertical_light = v_signal ? TRAFFIC_LIGHT_GREEN : TRAFFIC_LIGHT_RED,
    };
}

static double car_get_distance(const car_t* car, const traffic_light_t* light);
static double combination(const road_t* road, const traffic_light_t* light, const double* coeficients) {
    car_t* car;
    double sum = 0.0;
    double distance = 0.0;

    for (unsigned int i = 0; i < road_size(road); i++) {
        road_status_t _road = road_get(road, i, &car);
        if (_road != ROAD_STATUS_OK) {
            continue;
        }

        distance = car_get_distance(car, light);
        sum += coeficients[i] * (1/distance);
    }

    return sum;
}

static double car_get_distance(const car_t* car, const traffic_light_t* light) {
    unsigned int car_x = car_get_x(car);
    unsigned int car_y = car_get_y(car);
    unsigned int light_x = traffic_light_get_x(light);
    unsigned int light_y = traffic_light_get_y(light);

    return sqrt(pow(car_x - light_x, 2) + pow(car_y - light_y, 2));
}

static bool signal(const double combination, double threshold) {
    return combination > threshold;
}
