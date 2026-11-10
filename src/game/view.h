#ifndef VIEW_H
#define VIEW_H

#include <stdio.h>

#include "./entities/road.h"

/**
 * Terminal view: draws the two roads as a character grid. It only knows roads and cars,
 * not the game, so whoever runs the game decides what to hand it.
 *
 * The grid is VIEW_SIZE x VIEW_SIZE, centred on (0, 0), with +y up and +x to the right.
 * Cars outside of it are not drawn.
 */
#define VIEW_RADIUS 10
#define VIEW_SIZE (2 * VIEW_RADIUS + 1)

/** One character per cell. A cell holding cars of both roads is a collision. */
#define VIEW_EMPTY ' '
#define VIEW_CAR_HORIZONTAL '>'
#define VIEW_CAR_VERTICAL '^'
#define VIEW_COLLISION 'X'

/** ANSI sequence (cursor home + clear screen) written before every frame. */
#define VIEW_CLEAR_SCREEN "\033[H\033[2J"

typedef struct View view_t;

typedef enum view_status {
    /** The operation completed successfully. */
    VIEW_STATUS_OK,
    /** Memory allocation failed (view_create). */
    VIEW_STATUS_ENOMEM,
} view_status_t;

/**
 * On success, *view holds a new view that draws on `out` (e.g. stdout) and must be released
 * with view_destroy. The view does not own `out`: it never closes it.
 */
view_status_t view_create(FILE* out, view_t** view);
void view_destroy(view_t** view);

/**
 * Clears the screen and draws one frame: VIEW_SIZE lines of VIEW_SIZE characters.
 * A NULL road is drawn as empty. The roads are only read, and not kept after the call.
 */
void view_render(const view_t* view, const road_t* horizontal, const road_t* vertical);

#endif
