#include "agent.h"
#include "linear_combination.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    ai_t base; /* must be first */
    binarization_method_t binarization_method;
    double threshold;
    double* coeficients_h;
    double* coeficients_v;
    unsigned int length_h; /* number of coefficients in coeficients_h */
    unsigned int length_v;
} linear_combination_t;

static void linear_combination_destroy(ai_t* ai);
static game_command_t linear_combination_decide(ai_t* ai, const game_t* game);
const agent_ops_t linear_combination_ops = {
    .decide = linear_combination_decide,
    .destroy = linear_combination_destroy,
};

ai_status_t linear_combination_create(const config_t* config, ai_t** ai) {
    config_ai_player_t ai_config = config_get_ai_player(config);
    config_human_player_t world = config_get_human_player(config);

    linear_combination_t* self = calloc(1, sizeof(linear_combination_t));
    if (!self) {
        return AI_STATUS_ENOMEM;
    }

    /* The config owns its coefficient arrays and frees them in config_destroy, so keep copies. */
    self->coeficients_h = malloc(world.road_length_h * sizeof(double));
    self->coeficients_v = malloc(world.road_length_v * sizeof(double));
    if (!self->coeficients_h || !self->coeficients_v) {
        linear_combination_destroy((ai_t*)self);
        return AI_STATUS_ENOMEM;
    }
    memcpy(self->coeficients_h, ai_config.coeficients_h, world.road_length_h * sizeof(double));
    memcpy(self->coeficients_v, ai_config.coeficients_v, world.road_length_v * sizeof(double));

    self->length_h = world.road_length_h;
    self->length_v = world.road_length_v;
    self->base.ops = &linear_combination_ops;
    self->binarization_method = ai_config.binarization_method;
    self->threshold = ai_config.threshold;

    *ai = (ai_t*)self;
    return AI_STATUS_OK;
}

static void linear_combination_destroy(ai_t* ai) {
    linear_combination_t* self = (linear_combination_t*)ai;
    if (!self) {
        return;
    }

    free(self->coeficients_h);
    free(self->coeficients_v);
    free(self);
}

static double combination(const road_t* road, const traffic_light_t* light, const double* coeficients, unsigned int length);
static game_command_t choose_lights(game_command_t current, double difference, double margin);
static game_command_t linear_combination_decide(ai_t* ai, const game_t* game) {
    linear_combination_t* self = (linear_combination_t*) ai;

    const road_t* h_road = game_get_horizontal_road(game);
    const road_t* v_road = game_get_vertical_road(game);

    const traffic_light_t* h_light = game_get_horizontal_light(game);
    const traffic_light_t* v_light = game_get_vertical_light(game);

    double h_combination = combination(h_road, h_light, self->coeficients_h, self->length_h);
    double v_combination = combination(v_road, v_light, self->coeficients_v, self->length_v);

    return choose_lights(agent_current_command(game), h_combination - v_combination, self->threshold);
}

static bool car_has_passed(const car_t* car, const traffic_light_t* light);
static double car_get_distance(const car_t* car, const traffic_light_t* light);
/**
 * Sum of coeficients[k] * exp(-distance) over the cars still waiting for the light, where k is the
 * car's place in the queue (0 for the one closest to the light). Cars that already passed the light
 * stay on the road for a while but no longer depend on it, so they are skipped.
 */
static double combination(const road_t* road, const traffic_light_t* light, const double* coeficients, unsigned int length) {
    car_t* car;
    double sum = 0.0;
    unsigned int place = 0;

    for (unsigned int i = 0; i < road_size(road) && place < length; i++) {
        if (road_get(road, i, &car) != ROAD_STATUS_OK) {
            continue;
        }

        if (car_has_passed(car, light)) {
            continue;
        }

        sum += coeficients[place] * exp(-car_get_distance(car, light));
        place++;
    }

    return sum;
}

/** The light is the last cell before the crossing, so a car past it has a larger x (horizontal road) or y (vertical road). */
static bool car_has_passed(const car_t* car, const traffic_light_t* light) {
    return car_get_x(car) > traffic_light_get_x(light) || car_get_y(car) > traffic_light_get_y(light);
}

static double car_get_distance(const car_t* car, const traffic_light_t* light) {
    double dx = (double)car_get_x(car) - (double)traffic_light_get_x(light);
    double dy = (double)car_get_y(car) - (double)traffic_light_get_y(light);

    return hypot(dx, dy);
}

/**
 * Exactly one light is green after every tick. `difference` is the horizontal score minus the
 * vertical one, and `margin` (the threshold from the config) is how much better the other road must
 * be before the green light switches to it:
 *
 *  - difference >  margin: horizontal green, vertical red
 *  - difference < -margin: vertical green, horizontal red
 *  - otherwise: keep the lights as they are, which also stops them flickering when the scores are close.
 *    If the lights show the same color (e.g. both green at the start of the game), the better road
 *    gets the green, and a tie goes to the horizontal one.
 *
 * A negative margin would let both conditions hold at once, so it is treated as 0.
 */
static game_command_t choose_lights(game_command_t current, double difference, double margin) {
    if (margin < 0) {
        margin = 0;
    }

    bool horizontal_green;
    if (difference > margin) {
        horizontal_green = true;
    } else if (difference < -margin) {
        horizontal_green = false;
    } else if (current.horizontal_light != current.vertical_light) {
        return current;
    } else {
        horizontal_green = difference >= 0;
    }

    return (game_command_t) {
        .horizontal_light = horizontal_green ? TRAFFIC_LIGHT_GREEN : TRAFFIC_LIGHT_RED,
        .vertical_light = horizontal_green ? TRAFFIC_LIGHT_RED : TRAFFIC_LIGHT_GREEN,
    };
}
