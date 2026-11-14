#ifndef LINEAR_COMBINATION_H
#define LINEAR_COMBINATION_H

#include "ai.h"

/**
 * Each traffic light will be controlled by a linear combination like this:
 *
 *            R = f(a1 * x1 + a2 * x2 + ... + an * xn)
 *
 * Where:
 *
 *  - R is the signal to be binary (0 or 1) indicating whether the traffic light should be red or green.
 *  - a1, a2, ..., an are the coefficients representing the weights or costs associated with each variable.
 *  - x1, x2, ..., xn are the variables representing distances from each car to the traffic light (on the road).
 *  - f is a function that maps the linear combination to a binary output (0 or 1). This function could be a
 *    threshold function, where if the linear combination exceeds a certain threshold, the output is 1
 *    (green light), otherwise it is 0 (red light).
 */
ai_status_t linear_combination_create(const config_t* config, ai_t** ai);

#endif