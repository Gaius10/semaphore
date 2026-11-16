#ifndef LINEAR_COMBINATION_H
#define LINEAR_COMBINATION_H

#include "ai.h"

/**
 * Each road gets a score from a linear combination like this:
 *
 *            S = a1 * x1 + a2 * x2 + ... + an * xn
 *
 * Where:
 *
 *  - a1, a2, ..., an are the coefficients (one per place in the queue, 1 for the car closest to the light).
 *  - xk is the weight of the k-th car still waiting for the light: exp(-distance to the light), so 1 on the
 *    stop cell and fading with the distance. Cars that already passed the light are ignored.
 *
 * The lights are decided together from D = S_horizontal - S_vertical, so they are never both green or both
 * red after the first tick. With the threshold t from the config as a switching margin: D > t gives the green
 * to the horizontal road, D < -t to the vertical one, and otherwise the lights stay as they are (when both show
 * the same color, the busier road gets the green and a tie goes to the horizontal one).
 */
ai_status_t linear_combination_create(const config_t* config, ai_t** ai);

/**
 * The same agent, built straight from numbers instead of a config: `threshold` is the switching margin
 * t, and the coefficients are one per place in the queue of each road (`length_h` and `length_v` of
 * them; usually the road lengths, but any positive length works: cars beyond the last place count 0).
 * The training builds thousands of agents this way, with no file or parsing in between.
 *
 * The agent copies both arrays, so the caller can free or reuse them right away. Returns
 * AI_STATUS_EAGENT if an array is NULL or a length is 0, and AI_STATUS_ENOMEM if the memory runs out;
 * *ai is left untouched on failure. On success it must be released with ai_destroy.
 */
ai_status_t linear_combination_create_from(
    double threshold,
    const double* coeficients_h,
    unsigned int length_h,
    const double* coeficients_v,
    unsigned int length_v,
    ai_t** ai
);

#endif