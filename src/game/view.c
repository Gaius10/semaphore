#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "./view.h"
#include "./entities/car.h"

struct View {
    FILE* out;
};

typedef char grid_t[VIEW_SIZE][VIEW_SIZE];

static void plot_road(grid_t grid, const road_t* road, char symbol);
static bool to_cell(int x, int y, int* row, int* col);
static char merge_symbols(char current, char incoming);

view_status_t view_create(FILE* out, view_t** view) {
    view_t* new_view = malloc(sizeof(view_t));
    if (new_view == NULL) {
        return VIEW_STATUS_ENOMEM;
    }

    new_view->out = out;
    *view = new_view;

    return VIEW_STATUS_OK;
}

void view_destroy(view_t** view) {
    if (view == NULL || *view == NULL) {
        return;
    }

    free(*view);
    *view = NULL;
}

void view_render(const view_t* view, const road_t* horizontal, const road_t* vertical) {
    grid_t grid;
    memset(grid, VIEW_EMPTY, sizeof(grid));

    plot_road(grid, horizontal, VIEW_CAR_HORIZONTAL);
    plot_road(grid, vertical, VIEW_CAR_VERTICAL);

    fputs(VIEW_CLEAR_SCREEN, view->out);
    for (int row = 0; row < VIEW_SIZE; row++) {
        fwrite(grid[row], 1, VIEW_SIZE, view->out);
        fputc('\n', view->out);
    }

    fflush(view->out);
}

static void plot_road(grid_t grid, const road_t* road, char symbol) {
    if (road == NULL) {
        return;
    }

    for (int i = 0; i < road_size(road); i++) {
        car_t* car = NULL;
        if (road_get(road, i, &car) != ROAD_STATUS_OK) {
            // @todo Figure a better way to track this
            continue;
        }

        int row, col;
        if (!to_cell(car_get_x(car), car_get_y(car), &row, &col)) {
            continue; // Outside of the grid
        }

        grid[row][col] = merge_symbols(grid[row][col], symbol);
    }
}

/** Converts a world position to a grid cell. Returns false if the position is off the grid. */
static bool to_cell(int x, int y, int* row, int* col) {
    if (x < -VIEW_RADIUS || x > VIEW_RADIUS || y < -VIEW_RADIUS || y > VIEW_RADIUS) {
        return false;
    }

    *row = VIEW_RADIUS - y; // Row 0 is the top of the screen, which is y = +VIEW_RADIUS
    *col = x + VIEW_RADIUS;

    return true;
}

/** What a cell shows after a car drawn as `incoming` enters a cell that shows `current`. */
static char merge_symbols(char current, char incoming) {
    if (current == VIEW_EMPTY || current == incoming) {
        return incoming; // Free cell, or one more car of the same road
    }

    return VIEW_COLLISION; // Cars of different roads in the same cell
}
