#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <stdbool.h>

/**
 * The traffic light of one road. Opaque type: the struct is defined in traffic_light.c.
 *
 * (x, y) is the light's stop cell: the last cell of its road before the crossing. While the
 * light is red, a car on the stop cell waits there. Cars on any other cell ignore the light.
 */
typedef struct TrafficLight traffic_light_t;

typedef enum traffic_light_color {
    TRAFFIC_LIGHT_GREEN,
    TRAFFIC_LIGHT_RED,
} traffic_light_color_t;

typedef enum traffic_light_status {
    /** The operation completed successfully. */
    TRAFFIC_LIGHT_STATUS_OK,
    /** Memory allocation failed (traffic_light_create). */
    TRAFFIC_LIGHT_STATUS_ENOMEM,
} traffic_light_status_t;

/**
 * On success, *light holds a new light with stop cell (x, y) showing `color`. It must be
 * released with traffic_light_destroy.
 */
traffic_light_status_t traffic_light_create(int x, int y, traffic_light_color_t color, traffic_light_t** light);
void traffic_light_destroy(traffic_light_t** light);

int traffic_light_get_x(const traffic_light_t* light);
int traffic_light_get_y(const traffic_light_t* light);
traffic_light_color_t traffic_light_get_color(const traffic_light_t* light);

/** Green becomes red and red becomes green. Does nothing if `light` is NULL. */
void traffic_light_toggle(traffic_light_t* light);

/** True when a car at (x, y) must wait: the light is red and (x, y) is its stop cell. */
bool traffic_light_holds(const traffic_light_t* light, int x, int y);

#endif
