#include <stdlib.h>

#include "./traffic_light.h"

struct TrafficLight {
    int x;
    int y;
    traffic_light_color_t color;
};

traffic_light_status_t traffic_light_create(int x, int y, traffic_light_color_t color, traffic_light_t** light) {
    traffic_light_t* new_light = malloc(sizeof(traffic_light_t));
    if (new_light == NULL) {
        return TRAFFIC_LIGHT_STATUS_ENOMEM;
    }

    new_light->x = x;
    new_light->y = y;
    new_light->color = color;
    *light = new_light;

    return TRAFFIC_LIGHT_STATUS_OK;
}

void traffic_light_destroy(traffic_light_t** light) {
    if (light == NULL || *light == NULL) {
        return;
    }

    free(*light);
    *light = NULL;
}

int traffic_light_get_x(const traffic_light_t* light) {
    return light->x;
}

int traffic_light_get_y(const traffic_light_t* light) {
    return light->y;
}

traffic_light_color_t traffic_light_get_color(const traffic_light_t* light) {
    return light->color;
}

void traffic_light_toggle(traffic_light_t* light) {
    if (light == NULL) {
        return;
    }

    if (light->color == TRAFFIC_LIGHT_GREEN) {
        light->color = TRAFFIC_LIGHT_RED;
    } else {
        light->color = TRAFFIC_LIGHT_GREEN;
    }
}

bool traffic_light_holds(const traffic_light_t* light, int x, int y) {
    if (light == NULL) {
        return false;
    }

    return light->color == TRAFFIC_LIGHT_RED && light->x == x && light->y == y;
}
