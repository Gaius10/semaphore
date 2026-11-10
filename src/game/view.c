#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "./view.h"
#include "./entities/car.h"

struct View {
    FILE* out;
    unsigned int width;
    unsigned int height;
    /** Scratch frame, width * height cells, row by row. Redrawn on every render. */
    char* cells;
};

static void plot_road(view_t* view, const road_t* road, char symbol);
static bool in_grid(const view_t* view, int x, int y);
static unsigned int cell_index(const view_t* view, unsigned int x, unsigned int y);
static unsigned int cell_count(const view_t* view);
static char merge_symbols(char current, char incoming);

view_status_t view_create(FILE* out, unsigned int width, unsigned int height, view_t** view) {
    if (width == 0 || height == 0) {
        return VIEW_STATUS_EINVAL;
    }

    unsigned int cells_needed = width * height;
    if (cells_needed / width != height) {
        return VIEW_STATUS_ENOMEM; // width * height overflowed
    }

    view_t* new_view = malloc(sizeof(view_t));
    if (new_view == NULL) {
        return VIEW_STATUS_ENOMEM;
    }

    new_view->cells = malloc(cells_needed);
    if (new_view->cells == NULL) {
        free(new_view);
        return VIEW_STATUS_ENOMEM;
    }

    new_view->out = out;
    new_view->width = width;
    new_view->height = height;
    *view = new_view;

    return VIEW_STATUS_OK;
}

void view_destroy(view_t** view) {
    if (view == NULL || *view == NULL) {
        return;
    }

    free((*view)->cells);
    free(*view);
    *view = NULL;
}

void view_render(view_t* view, const road_t* horizontal, const road_t* vertical) {
    memset(view->cells, VIEW_EMPTY, cell_count(view));

    plot_road(view, horizontal, VIEW_CAR_HORIZONTAL);
    plot_road(view, vertical, VIEW_CAR_VERTICAL);

    fputs(VIEW_CLEAR_SCREEN, view->out);
    for (unsigned int y = 0; y < view->height; y++) {
        const char* row = &view->cells[cell_index(view, 0, y)];

        fwrite(row, sizeof(char), view->width, view->out);
        fputc(VIEW_EDGE, view->out);
        fputc('\n', view->out);
    }

    fflush(view->out);
}

static void plot_road(view_t* view, const road_t* road, char symbol) {
    if (road == NULL) {
        return;
    }

    for (int i = 0; i < road_size(road); i++) {
        car_t* car = NULL;
        if (road_get(road, i, &car) != ROAD_STATUS_OK) {
            // @todo Figure a better way to track this
            continue;
        }

        int x = car_get_x(car);
        int y = car_get_y(car);
        if (!in_grid(view, x, y)) {
            continue;
        }

        char* cell = &view->cells[cell_index(view, (unsigned int) x, (unsigned int) y)];
        *cell = merge_symbols(*cell, symbol);
    }
}

static bool in_grid(const view_t* view, int x, int y) {
    // Negative coordinates are checked first, so the casts below never see them
    return x >= 0 && (unsigned int) x < view->width && y >= 0 && (unsigned int) y < view->height;
}

/** Position of cell (x, y) in `cells`, which holds the grid row by row. (x, y) must be in the grid. */
static unsigned int cell_index(const view_t* view, unsigned int x, unsigned int y) {
    return y * view->width + x;
}

static unsigned int cell_count(const view_t* view) {
    return view->width * view->height;
}

/** What a cell shows after a car drawn as `incoming` enters a cell that shows `current`. */
static char merge_symbols(char current, char incoming) {
    if (current == VIEW_EMPTY || current == incoming) {
        return incoming; // Free cell, or one more car of the same road
    }

    return VIEW_COLLISION; // Cars of different roads in the same cell
}
