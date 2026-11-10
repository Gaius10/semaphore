#ifndef VIEW_H
#define VIEW_H

#include <stdio.h>

#include "./entities/road.h"
#include "./entities/traffic_light.h"

/**
 * Terminal view: draws the two roads and their traffic lights as a character grid. It only knows
 * roads, cars and lights, not the game, so whoever runs the game decides what to hand it.
 *
 * The grid is width x height cells, set when the view is created (it comes from the config).
 * Like the screen, (0, 0) is the top-left cell, x grows to the right and y grows downwards.
 * Cars outside of the grid are not drawn.
 */

/**
 * One character per cell. The symbols point where the cars travel: horizontal cars move
 * right and vertical cars move down. A cell holding cars of both roads is a collision.
 */
#define VIEW_EMPTY ' '
#define VIEW_CAR_HORIZONTAL '>'
#define VIEW_CAR_VERTICAL 'v'
#define VIEW_COLLISION 'X'

/**
 * A light is drawn next to its stop cell, on the right of the drivers: below the stop cell for
 * the horizontal road (its cars move right) and left of it for the vertical road (its cars move
 * down). A light is drawn over whatever else is in that cell.
 */
#define VIEW_LIGHT_GREEN 'G'
#define VIEW_LIGHT_RED 'R'

/** Written after the last cell of every line, to show where the grid ends. */
#define VIEW_EDGE '|'

/** ANSI sequence (cursor home + clear screen) written before every frame. */
#define VIEW_CLEAR_SCREEN "\033[H\033[2J"

typedef struct View view_t;

typedef enum view_status {
    /** The operation completed successfully. */
    VIEW_STATUS_OK,
    /** Memory allocation failed (view_create). */
    VIEW_STATUS_ENOMEM,
    /** The width or the height is zero (view_create). */
    VIEW_STATUS_EINVAL,
} view_status_t;

/**
 * On success, *view holds a new view of width x height cells that draws on `out` (e.g. stdout)
 * and must be released with view_destroy. The view does not own `out`: it never closes it.
 */
view_status_t view_create(FILE* out, unsigned int width, unsigned int height, view_t** view);
void view_destroy(view_t** view);

/**
 * Clears the screen and draws one frame: `height` lines of `width` cells, each followed by
 * VIEW_EDGE. A NULL road is drawn as empty and a NULL light is not drawn. The roads and lights
 * are only read, and not kept after the call.
 */
void view_render(
    view_t* view,
    const road_t* horizontal,
    const road_t* vertical,
    const traffic_light_t* horizontal_light,
    const traffic_light_t* vertical_light
);

#endif
