#define _DEFAULT_SOURCE /* ioctl(TIOCGWINSZ), isatty and clock_gettime are hidden under strict -std=c2x */

#include "dashboard.h"

#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define MAX_WIDTH 100
#define WORKER_CELL_WIDTH 21 /* "w00 [██████] 99/99" and some air */
#define WORKER_BAR_WIDTH 6
#define FIXED_LINES 24 /* every line of the panel but the workers' */

static const char* const LEVELS[8] = { "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█" };

#define CLEAR_LINE "\x1b[K"
#define HIDE_CURSOR "\x1b[?25l"
#define SHOW_CURSOR "\x1b[?25h"
#define BOLD "\x1b[1m"
#define DIM "\x1b[2m"
#define GREEN "\x1b[32m"
#define RED "\x1b[31m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

/** The terminal a live dashboard hid the cursor of, for dashboard_emergency_restore (-1: none). */
static volatile sig_atomic_t cursor_hidden_on = -1;

typedef struct text {
    char* data;
    size_t length;
    size_t capacity;
    bool failed;
} text_t;

struct dashboard {
    FILE* out;
    dashboard_style_t style;
    unsigned int width;
    bool color;
    dashboard_info_t info;
    char csv_path[512];
    struct timespec start;

    /* One entry per finished generation. */
    unsigned int completed;
    double* champion_history;
    double* mean_history;
    double* worst_history;
    double best_ever;
    unsigned int best_ever_generation;
    double last_cars_passed;
    double last_crashes;
    double* last_genome;

    /* The generation being played. */
    bool evaluating;
    unsigned int total;
    unsigned int done;
    unsigned int* assigned;
    unsigned int* finished;

    bool finished_run;  /* the run is over... */
    bool run_completed; /* ...having done every generation (not stopped early) */
    unsigned int drawn_lines;
};

static void text_add(text_t* text, const char* format, ...);
static void build_frame(const dashboard_t* d, text_t* text);
static void draw(dashboard_t* d);
static void print_plain_header(const dashboard_t* d);
static void print_plain_generation(const dashboard_t* d, const dashboard_generation_t* generation);
static double seconds_since(const struct timespec* start);
static unsigned int worker_rows(unsigned int workers, unsigned int width);

/* ---------------------------------------------------------------- pure helpers */

size_t dashboard_bar(char* out, size_t size, double fraction, unsigned int width) {
    out[0] = '\0';
    if (size < (size_t)width * 3 + 1) {
        return 0;
    }

    if (!(fraction > 0.0)) { /* also catches NaN */
        fraction = 0.0;
    }
    if (fraction > 1.0) {
        fraction = 1.0;
    }

    unsigned int filled = (unsigned int)floor(fraction * width + 1e-9);
    char* end = out;
    for (unsigned int i = 0; i < width; i++) {
        const char* cell = i < filled ? "█" : "░";
        memcpy(end, cell, 3);
        end += 3;
    }
    *end = '\0';
    return (size_t)(end - out);
}

size_t dashboard_sparkline(char* out, size_t size, const double* values, unsigned int count, unsigned int width) {
    out[0] = '\0';
    unsigned int shown = count < width ? count : width;
    if (size < (size_t)shown * 3 + 1) {
        return 0;
    }

    const double* last = &values[count - shown];
    double low = INFINITY;
    double high = -INFINITY;
    for (unsigned int i = 0; i < shown; i++) {
        low = fmin(low, last[i]);
        high = fmax(high, last[i]);
    }

    char* end = out;
    for (unsigned int i = 0; i < shown; i++) {
        unsigned int level = 3; /* a flat series sits in the middle */
        if (high > low) {
            level = (unsigned int)lround((last[i] - low) / (high - low) * 7.0);
        }
        memcpy(end, LEVELS[level > 7 ? 7 : level], 3);
        end += 3;
    }
    *end = '\0';
    return (size_t)(end - out);
}

size_t dashboard_levels(char* out, size_t size, const double* values, unsigned int count, double max) {
    out[0] = '\0';
    if (size < (size_t)count * 3 + 1) {
        return 0;
    }

    char* end = out;
    for (unsigned int i = 0; i < count; i++) {
        double fraction = max > 0.0 ? values[i] / max : 0.0;
        if (!(fraction > 0.0)) {
            fraction = 0.0;
        }
        if (fraction > 1.0) {
            fraction = 1.0;
        }
        memcpy(end, LEVELS[lround(fraction * 7.0)], 3);
        end += 3;
    }
    *end = '\0';
    return (size_t)(end - out);
}

int dashboard_format_delta(char* out, size_t size, double delta) {
    if (isnan(delta) || fabs(delta) < 0.005) {
        return snprintf(out, size, "= 0.00");
    }

    return delta > 0 ? snprintf(out, size, "▲ +%.2f", delta) : snprintf(out, size, "▼ -%.2f", fabs(delta));
}

int dashboard_format_duration(char* out, size_t size, double seconds) {
    if (isnan(seconds) || seconds < 0.0) {
        return snprintf(out, size, "--");
    }

    if (seconds < 59.95) {
        return snprintf(out, size, "%.1fs", seconds);
    }

    unsigned long whole = (unsigned long)seconds;
    if (whole < 3600) {
        return snprintf(out, size, "%lum%02lus", whole / 60, whole % 60);
    }

    return snprintf(out, size, "%luh%02lum", whole / 3600, (whole % 3600) / 60);
}

static unsigned int worker_rows(unsigned int workers, unsigned int width) {
    unsigned int per_row = width / WORKER_CELL_WIDTH;
    if (per_row == 0) {
        per_row = 1;
    }

    return (workers + per_row - 1) / per_row;
}

unsigned int dashboard_frame_height(unsigned int workers, unsigned int width) {
    return FIXED_LINES + worker_rows(workers, width);
}

dashboard_style_t dashboard_choose_style(
    bool is_tty,
    bool dumb_terminal,
    unsigned int columns,
    unsigned int rows,
    unsigned int workers
) {
    if (!is_tty || dumb_terminal || columns < DASHBOARD_MIN_WIDTH) {
        return DASHBOARD_PLAIN;
    }

    unsigned int width = columns < MAX_WIDTH ? columns : MAX_WIDTH;
    /* One more line than the panel, so that the last line does not scroll the screen. */
    return rows >= dashboard_frame_height(workers, width) + 1 ? DASHBOARD_LIVE : DASHBOARD_PLAIN;
}

/* ---------------------------------------------------------------- the dashboard */

dashboard_style_t dashboard_detect_style(FILE* out, unsigned int workers, unsigned int* width) {
    *width = 0;

    int fd = fileno(out);
    const char* term = getenv("TERM");
    bool dumb = term == NULL || strcmp(term, "dumb") == 0;
    struct winsize size;
    bool is_tty = fd >= 0 && isatty(fd) && ioctl(fd, TIOCGWINSZ, &size) == 0;

    dashboard_style_t style = dashboard_choose_style(is_tty, dumb, is_tty ? size.ws_col : 0, is_tty ? size.ws_row : 0, workers);
    if (style == DASHBOARD_LIVE) {
        *width = size.ws_col < MAX_WIDTH ? size.ws_col : MAX_WIDTH;
    }

    return style;
}

bool dashboard_create(FILE* out, dashboard_style_t style, unsigned int width, const dashboard_info_t* info, dashboard_t** dashboard) {
    dashboard_t* d = calloc(1, sizeof(dashboard_t));
    if (d == NULL) {
        return false;
    }

    unsigned int genome_length = 1 + info->length_h + info->length_v;
    double* history = calloc((size_t)info->generations * 3, sizeof(double));
    d->last_genome = calloc(genome_length, sizeof(double));
    d->assigned = calloc(info->workers, sizeof(unsigned int));
    d->finished = calloc(info->workers, sizeof(unsigned int));
    if (history == NULL || d->last_genome == NULL || d->assigned == NULL || d->finished == NULL) {
        free(history);
        free(d->last_genome);
        free(d->assigned);
        free(d->finished);
        free(d);
        return false;
    }

    d->champion_history = history;
    d->mean_history = history + info->generations;
    d->worst_history = history + 2 * (size_t)info->generations;

    d->out = out;
    d->style = style;
    d->width = width < MAX_WIDTH ? width : MAX_WIDTH;
    d->color = getenv("NO_COLOR") == NULL;
    d->info = *info;
    snprintf(d->csv_path, sizeof(d->csv_path), "%s", info->csv_path);
    d->info.csv_path = d->csv_path;
    clock_gettime(CLOCK_MONOTONIC, &d->start);

    /* Before the first progress report, show the shares the evaluator will hand out, all still to do. */
    d->total = info->population_size;
    unsigned int started = info->workers < info->population_size ? info->workers : info->population_size;
    for (unsigned int w = 0; w < started; w++) {
        d->assigned[w] = (info->population_size - w + started - 1) / started;
    }

    if (style == DASHBOARD_LIVE) {
        fputs(HIDE_CURSOR, out);
        cursor_hidden_on = fileno(out);
        draw(d);
    } else {
        print_plain_header(d);
    }

    *dashboard = d;
    return true;
}

void dashboard_destroy(dashboard_t** dashboard) {
    if (dashboard == NULL || *dashboard == NULL) {
        return;
    }

    dashboard_t* d = *dashboard;
    if (d->style == DASHBOARD_LIVE && cursor_hidden_on == fileno(d->out)) {
        fputs(SHOW_CURSOR, d->out);
        fflush(d->out);
        cursor_hidden_on = -1;
    }

    free(d->champion_history); /* the three histories are one block */
    free(d->last_genome);
    free(d->assigned);
    free(d->finished);
    free(d);
    *dashboard = NULL;
}

void dashboard_update_progress(dashboard_t* d, const evaluator_progress_t* progress) {
    if (d->style != DASHBOARD_LIVE) {
        return;
    }

    d->evaluating = true;
    d->total = progress->total;
    d->done = progress->done;
    for (unsigned int w = 0; w < d->info.workers && w < progress->workers; w++) {
        d->assigned[w] = progress->assigned[w];
        d->finished[w] = progress->finished[w];
    }
    draw(d);
}

void dashboard_add_generation(dashboard_t* d, const dashboard_generation_t* generation) {
    if (d->completed < d->info.generations) {
        d->champion_history[d->completed] = generation->champion_fitness;
        d->mean_history[d->completed] = generation->mean_fitness;
        d->worst_history[d->completed] = generation->worst_fitness;
    }

    if (d->completed == 0 || generation->champion_fitness > d->best_ever) {
        d->best_ever = generation->champion_fitness;
        d->best_ever_generation = generation->index;
    }

    d->completed++;
    d->last_cars_passed = generation->champion_cars_passed;
    d->last_crashes = generation->champion_crashes;
    memcpy(d->last_genome, generation->champion_genome, (1 + d->info.length_h + d->info.length_v) * sizeof(double));
    d->evaluating = false;

    if (d->style == DASHBOARD_LIVE) {
        draw(d);
    } else {
        print_plain_generation(d, generation);
    }
}

void dashboard_finish(dashboard_t* d, bool completed) {
    d->finished_run = true;
    d->run_completed = completed;
    d->evaluating = false;

    if (d->style == DASHBOARD_LIVE) {
        draw(d);
        fputs(SHOW_CURSOR, d->out);
        fflush(d->out);
        cursor_hidden_on = -1;
    }
}

void dashboard_emergency_restore(void) {
    int fd = cursor_hidden_on;
    if (fd >= 0) {
        ssize_t ignored = write(fd, SHOW_CURSOR, sizeof(SHOW_CURSOR) - 1);
        (void)ignored;
    }
}

/* ---------------------------------------------------------------- plain style */

static void print_plain_header(const dashboard_t* d) {
    const dashboard_info_t* in = &d->info;

    fprintf(
        d->out,
        "Training: %u candidates x %u generations, %u episodes of %u ticks each, seed %u, %u worker%s.\n",
        in->population_size,
        in->generations,
        in->episodes_per_candidate,
        in->episode_ticks,
        in->seed,
        in->workers,
        in->workers == 1 ? "" : "s"
    );
    fprintf(d->out, "Results: %s\n\n", in->csv_path);
    fflush(d->out);
}

static void print_plain_generation(const dashboard_t* d, const dashboard_generation_t* generation) {
    int digits = snprintf(NULL, 0, "%u", d->info.generations);

    fprintf(
        d->out,
        "generation %*u/%u  champion %8.2f  mean %8.2f  worst %8.2f  (cars %.1f, crashes %.2f)  %6.1fs\n",
        digits,
        generation->index + 1,
        d->info.generations,
        generation->champion_fitness,
        generation->mean_fitness,
        generation->worst_fitness,
        generation->champion_cars_passed,
        generation->champion_crashes,
        seconds_since(&d->start)
    );
    fflush(d->out);
}

/* ---------------------------------------------------------------- live style */

static void text_add(text_t* text, const char* format, ...) {
    if (text->failed) {
        return;
    }

    va_list args;
    va_start(args, format);
    int needed = vsnprintf(NULL, 0, format, args);
    va_end(args);
    if (needed < 0) {
        text->failed = true;
        return;
    }

    if (text->length + (size_t)needed + 1 > text->capacity) {
        size_t capacity = text->capacity == 0 ? 4096 : text->capacity;
        while (text->length + (size_t)needed + 1 > capacity) {
            capacity *= 2;
        }

        char* grown = realloc(text->data, capacity);
        if (grown == NULL) {
            text->failed = true;
            return;
        }
        text->data = grown;
        text->capacity = capacity;
    }

    va_start(args, format);
    vsnprintf(text->data + text->length, text->capacity - text->length, format, args);
    va_end(args);
    text->length += (size_t)needed;
}

/** Writes the panel over the one drawn before (the cursor goes back up to its first line). */
static void draw(dashboard_t* d) {
    text_t text = { 0 };

    if (d->drawn_lines > 0) {
        text_add(&text, "\x1b[%uA\r", d->drawn_lines);
    }
    build_frame(d, &text);

    if (!text.failed) {
        fwrite(text.data, 1, text.length, d->out);
        fflush(d->out);
        d->drawn_lines = dashboard_frame_height(d->info.workers, d->width);
    }
    free(text.data);
}

/** The color code, or nothing when colors are off. */
static const char* paint(const dashboard_t* d, const char* code) {
    return d->color ? code : "";
}

static const char* trend_color(const dashboard_t* d, double delta) {
    if (isnan(delta) || fabs(delta) < 0.005) {
        return paint(d, DIM);
    }

    return paint(d, delta > 0 ? GREEN : RED);
}

/** One line of the fitness panel: label, value and the change from the generation before. */
static void fitness_line(text_t* text, const dashboard_t* d, const char* label, const double* history) {
    char delta[32] = "";

    if (d->completed == 0) {
        text_add(text, "  %-9s       --" CLEAR_LINE "\n", label);
        return;
    }

    double now = history[d->completed - 1];
    double change = d->completed > 1 ? now - history[d->completed - 2] : 0.0;
    if (d->completed > 1) {
        dashboard_format_delta(delta, sizeof(delta), change);
    }
    text_add(text, "  %-9s %8.2f  %s%s%s" CLEAR_LINE "\n", label, now, trend_color(d, change), delta, paint(d, RESET));
}

static void build_frame(const dashboard_t* d, text_t* text) {
    const dashboard_info_t* in = &d->info;
    char bar[MAX_WIDTH * 3 + 1];
    char duration[32];
    char eta[32];
    char spark[MAX_WIDTH * 3 + 1];

    double elapsed = seconds_since(&d->start);
    double fraction = 0.0;
    if (d->finished_run && d->run_completed) {
        fraction = 1.0;
    } else if (in->generations > 0) {
        double current = d->evaluating && d->total > 0 ? (double)d->done / d->total : 0.0;
        fraction = ((double)d->completed + current) / in->generations;
    }
    unsigned int shown_generation = d->completed + (d->finished_run || d->completed >= in->generations ? 0 : 1);
    if (shown_generation > in->generations) {
        shown_generation = in->generations;
    }

    text_add(text, "%s%sSemaphore training%s" CLEAR_LINE "\n", paint(d, BOLD), paint(d, CYAN), paint(d, RESET));
    text_add(text, "%u candidates x %u generations" CLEAR_LINE "\n", in->population_size, in->generations);
    text_add(text, "%u episodes x %u ticks each" CLEAR_LINE "\n", in->episodes_per_candidate, in->episode_ticks);
    text_add(text, "%u worker%s | seed %u | roads %u and %u cells" CLEAR_LINE "\n", in->workers, in->workers == 1 ? "" : "s", in->seed, in->length_h, in->length_v);

    /* The path is cut from the left, so that its end (the file name) stays. */
    size_t room = d->width > 12 ? d->width - 12 : 0;
    size_t path_length = strlen(in->csv_path);
    bool cut = path_length > room;
    text_add(text, "%sresults:%s %s%s" CLEAR_LINE "\n", paint(d, DIM), paint(d, RESET), cut ? "..." : "", in->csv_path + (cut ? path_length - room + 3 : 0));
    text_add(text, CLEAR_LINE "\n");

    dashboard_bar(bar, sizeof(bar), fraction, 20);
    text_add(text, "%sGeneration %u/%u%s  %s %3.0f%%" CLEAR_LINE "\n", paint(d, BOLD), shown_generation, in->generations, paint(d, RESET), bar, floor(fraction * 100.0));

    double remaining = fraction > 0.001 ? elapsed * (1.0 - fraction) / fraction : NAN;
    dashboard_format_duration(duration, sizeof(duration), elapsed);
    dashboard_format_duration(eta, sizeof(eta), d->finished_run ? (d->run_completed ? 0.0 : NAN) : remaining);
    text_add(text, "elapsed %s | left %s" CLEAR_LINE "\n", duration, eta);
    text_add(text, CLEAR_LINE "\n");

    /* The last progress stays on show between generations (full), until the next one starts. */
    dashboard_bar(bar, sizeof(bar), d->total > 0 ? (double)d->done / d->total : 0.0, 20);
    text_add(text, "Playing candidates  %s %u/%u" CLEAR_LINE "\n", bar, d->done, d->total);

    unsigned int per_row = d->width / WORKER_CELL_WIDTH;
    if (per_row == 0) {
        per_row = 1;
    }
    for (unsigned int w = 0; w < in->workers; w++) {
        unsigned int assigned = d->assigned[w];
        unsigned int finished = d->finished[w];
        dashboard_bar(bar, sizeof(bar), assigned > 0 ? (double)finished / assigned : 0.0, WORKER_BAR_WIDTH);
        text_add(text, "%sw%02u [%s] %2u/%-2u%s", paint(d, DIM), w, bar, finished, assigned, paint(d, RESET));
        bool end_of_row = (w + 1) % per_row == 0 || w + 1 == in->workers;
        text_add(text, end_of_row ? CLEAR_LINE "\n" : "   ");
    }
    text_add(text, CLEAR_LINE "\n");

    text_add(text, "%sFitness%s" CLEAR_LINE "\n", paint(d, BOLD), paint(d, RESET));
    if (d->completed == 0) {
        text_add(text, "  %-9s       --" CLEAR_LINE "\n", "best ever");
    } else {
        text_add(text, "  %-9s %8.2f  %s(generation %u)%s" CLEAR_LINE "\n", "best ever", d->best_ever, paint(d, DIM), d->best_ever_generation + 1, paint(d, RESET));
    }
    fitness_line(text, d, "champion", d->champion_history);
    fitness_line(text, d, "mean", d->mean_history);
    fitness_line(text, d, "worst", d->worst_history);
    text_add(text, CLEAR_LINE "\n");

    unsigned int spark_width = d->width > 14 ? d->width - 14 : 1;
    if (spark_width > in->generations) {
        spark_width = in->generations;
    }
    dashboard_sparkline(spark, sizeof(spark), d->champion_history, d->completed, spark_width);
    text_add(text, "  champion  %s%s%s" CLEAR_LINE "\n", paint(d, GREEN), spark, paint(d, RESET));
    dashboard_sparkline(spark, sizeof(spark), d->mean_history, d->completed, spark_width);
    text_add(text, "  mean      %s%s%s" CLEAR_LINE "\n", paint(d, CYAN), spark, paint(d, RESET));
    text_add(text, CLEAR_LINE "\n");

    text_add(text, "%sChampion of the last generation%s" CLEAR_LINE "\n", paint(d, BOLD), paint(d, RESET));
    if (d->completed == 0) {
        text_add(text, "  --" CLEAR_LINE "\n");
        text_add(text, "  horizontal" CLEAR_LINE "\n");
        text_add(text, "  vertical" CLEAR_LINE "\n");
    } else {
        dashboard_bar(bar, sizeof(bar), d->last_genome[0] / in->threshold_max, 10);
        text_add(text, "  cars %.1f | crashes %.2f | threshold %.3f %s" CLEAR_LINE "\n", d->last_cars_passed, d->last_crashes, d->last_genome[0], bar);

        unsigned int room_genes = d->width > 16 ? d->width - 16 : 1;
        unsigned int shown_h = in->length_h < room_genes ? in->length_h : room_genes;
        unsigned int shown_v = in->length_v < room_genes ? in->length_v : room_genes;
        char genes[MAX_WIDTH * 3 + 1];
        dashboard_levels(genes, sizeof(genes), &d->last_genome[1], shown_h, in->coeficient_max);
        text_add(text, "  horizontal  %s%s" CLEAR_LINE "\n", genes, shown_h < in->length_h ? "..." : "");
        dashboard_levels(genes, sizeof(genes), &d->last_genome[1 + in->length_h], shown_v, in->coeficient_max);
        text_add(text, "  vertical    %s%s" CLEAR_LINE "\n", genes, shown_v < in->length_v ? "..." : "");
    }
}

static double seconds_since(const struct timespec* start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)(now.tv_sec - start->tv_sec) + (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}
