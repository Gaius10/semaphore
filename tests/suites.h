#ifndef SUITES_H
#define SUITES_H

#include <check.h>

Suite* car_suite(void);
Suite* road_suite(void);
Suite* config_suite(void);
Suite* view_suite(void);
Suite* game_suite(void);
Suite* input_suite(void);
Suite* traffic_light_suite(void);
Suite* ai_suite(void);
Suite* noop_suite(void);
Suite* fixed_toggle_suite(void);
Suite* linear_combination_suite(void);
Suite* rng_suite(void);
Suite* genetic_suite(void);
Suite* evaluator_suite(void);
Suite* training_suite(void);

#endif
