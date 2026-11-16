#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "evaluator.h"

/**
 * What training shows on the terminal. Two styles:
 *
 *  - LIVE: a panel redrawn in place with ANSI escapes (the project already uses them for the game
 *    view): the run summary, the progress of the whole run and of every worker, the fitness of the
 *    last generation with its change from the one before, sparklines of the champion and mean fitness
 *    over the run, and the genome of the champion drawn as bars.
 *  - PLAIN: one text line per generation and no escape codes, for output that is redirected to a file
 *    or a pipe, and for terminals too small for the panel.
 *
 * The functions at the top are pure (they only format text), so they can be tested on their own.
 */

/* ------------------------------------------------------------------ pure helpers */

/**
 * A progress bar of `width` cells: "█" for the filled part and "░" for the rest, filled up to
 * `fraction` (0 to 1; anything outside is clamped) rounded down, so a full bar means done. Writes a
 * terminated string to `out` and returns its length in bytes, or 0 (with `out` set to "") if it does
 * not fit in `size`.
 */
size_t dashboard_bar(char* out, size_t size, double fraction, unsigned int width);

/**
 * One block character ("▁▂▃▄▅▆▇█", 8 levels) per value, for the last `width` of the `count` values,
 * scaled between the smallest and the largest of those. A flat series is drawn at the middle level.
 * Same return value as dashboard_bar.
 */
size_t dashboard_sparkline(char* out, size_t size, const double* values, unsigned int count, unsigned int width);

/**
 * One block character per value, scaled from 0 (lowest block) to `max` (full block), for showing
 * numbers that have a known range, such as the genes. Values outside [0, max] are clamped. Same return
 * value as dashboard_bar.
 */
size_t dashboard_levels(char* out, size_t size, const double* values, unsigned int count, double max);

/**
 * "▲ +1.33" for a rise, "▼ -1.33" for a fall and "= 0.00" for a change that rounds to nothing.
 * Returns what snprintf returns.
 */
int dashboard_format_delta(char* out, size_t size, double delta);

/** "12.3s" under a minute, "1m05s" under an hour, "1h02m" beyond, and "--" for a negative or NaN time. */
int dashboard_format_duration(char* out, size_t size, double seconds);

typedef enum dashboard_style {
    DASHBOARD_PLAIN,
    DASHBOARD_LIVE,
} dashboard_style_t;

/**
 * Which style fits: LIVE only on a real terminal (`is_tty`) that is not "dumb" and is at least
 * DASHBOARD_MIN_WIDTH wide and tall enough for the panel for that many workers, else PLAIN.
 */
dashboard_style_t dashboard_choose_style(
    bool is_tty,
    bool dumb_terminal,
    unsigned int columns,
    unsigned int rows,
    unsigned int workers
);

#define DASHBOARD_MIN_WIDTH 70

/** How many lines the live panel takes for that many workers when it is `width` columns wide. */
unsigned int dashboard_frame_height(unsigned int workers, unsigned int width);

/* ------------------------------------------------------------------ the dashboard */

typedef struct dashboard dashboard_t;

/** What does not change during the run. */
typedef struct dashboard_info {
    unsigned int population_size;
    unsigned int generations;
    unsigned int episodes_per_candidate;
    unsigned int episode_ticks;
    unsigned int workers;
    unsigned int seed;
    unsigned int length_h;
    unsigned int length_v;
    double coeficient_max;
    double threshold_max;
    /** Where the results file is. Copied. */
    const char* csv_path;
} dashboard_info_t;

/** The champion of a generation, as the dashboard wants it. */
typedef struct dashboard_generation {
    unsigned int index;
    double champion_fitness;
    double mean_fitness;
    double worst_fitness;
    double champion_cars_passed;
    double champion_crashes;
    /** 1 + length_h + length_v numbers (see candidate.h). */
    const double* champion_genome;
} dashboard_generation_t;

/**
 * The style to use for `out`: LIVE if it is a terminal that is big enough (its size is written to
 * *width, capped at 100), else PLAIN.
 */
dashboard_style_t dashboard_detect_style(FILE* out, unsigned int workers, unsigned int* width);

/**
 * Starts the dashboard on `out`, with a panel `width` columns wide when LIVE. In PLAIN style it prints
 * the run summary right away. In LIVE style it hides the cursor, so a run that does not end through
 * dashboard_finish or dashboard_destroy must call dashboard_emergency_restore.
 * On success, *dashboard must be released with dashboard_destroy.
 */
bool dashboard_create(FILE* out, dashboard_style_t style, unsigned int width, const dashboard_info_t* info, dashboard_t** dashboard);

/** Shows the cursor again if it was hidden, and frees the dashboard. */
void dashboard_destroy(dashboard_t** dashboard);

/** The workers' progress in the generation being played (LIVE redraws the panel; PLAIN ignores it). */
void dashboard_update_progress(dashboard_t* dashboard, const evaluator_progress_t* progress);

/** A generation is done: LIVE redraws the panel, PLAIN prints its line. */
void dashboard_add_generation(dashboard_t* dashboard, const dashboard_generation_t* generation);

/**
 * The run is over: LIVE draws a last panel and leaves it on the screen, with the cursor shown and
 * below it, so whatever is printed next goes underneath. `completed` says whether every generation
 * was done (the panel then shows 100%) or the run was stopped early (it shows how far it got).
 */
void dashboard_finish(dashboard_t* dashboard, bool completed);

/**
 * Shows the cursor if a LIVE dashboard hid it. Safe to call from a signal handler (it only uses
 * write), and a no-op otherwise.
 */
void dashboard_emergency_restore(void);

#endif
