#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <stdint.h>

#define TL_RED 0
#define TL_GREEN 1

typedef struct traffic_light_t {
    int pos_x;
    int pos_y;
    uint8_t h_state;
    uint8_t v_state;
} traffic_light_t;

void tl_init(traffic_light_t* traffic_light, int x, int y);
void tl_toggle_horizontal(traffic_light_t* traffic_light);
void tl_toggle_vertical(traffic_light_t* traffic_light);

#endif
