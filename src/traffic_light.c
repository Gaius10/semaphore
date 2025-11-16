#include "../lib/traffic_light.h"

void tl_init(traffic_light_t* traffic_light, int x, int y) {
    traffic_light->pos_x = x;
    traffic_light->pos_y = y;
    traffic_light->h_state = TL_RED;
    traffic_light->v_state = TL_RED;
}

void tl_toggle_horizontal(traffic_light_t* traffic_light) {
    traffic_light->h_state = !traffic_light->h_state;
}

void tl_toggle_vertical(traffic_light_t* traffic_light) {
    traffic_light->v_state = !traffic_light->v_state;
}
