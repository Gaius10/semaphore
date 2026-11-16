#ifndef RESULTS_CSV_H
#define RESULTS_CSV_H

#include <stddef.h>
#include <time.h>

/**
 * The results file of a training run: a CSV with a header and one row per generation, with the
 * champion of that generation in full, so the evolution can be plotted (or a champion re-scored)
 * later with pandas, gnuplot, a spreadsheet...
 *
 * Columns, in order:
 *
 *     generation,champion_fitness,mean_fitness,worst_fitness,
 *     champion_cars_passed,champion_crashes,threshold,h0..h<length_h-1>,v0..v<length_v-1>
 *
 * `generation` starts at 0 (it is the number the traffic seeds are made from). The champion's cars
 * passed and crashes are averages over the episodes. Numbers are written with 17 significant digits,
 * so they read back exactly. Every row is flushed as soon as it is written, so a run that is
 * interrupted leaves a usable file.
 */
typedef struct results_csv results_csv_t;

typedef enum results_csv_status {
    RESULTS_CSV_STATUS_OK,
    RESULTS_CSV_STATUS_ENOMEM,
    RESULTS_CSV_STATUS_EEXIST, // The file is already there: it is never overwritten.
    RESULTS_CSV_STATUS_EIO,    // The file could not be created or written.
} results_csv_status_t;

/** One row: what the champion of a generation did, and the genome that did it. */
typedef struct results_row {
    unsigned int generation;
    double champion_fitness;
    double mean_fitness;
    double worst_fitness;
    double champion_cars_passed;
    double champion_crashes;
    /** The champion's genome: 1 + length_h + length_v numbers (see candidate.h). */
    const double* genome;
} results_row_t;

/**
 * The name of a new results file in `dir`, from the time `when`: dir/training_YYYYmmdd_HHMMSS.csv in
 * local time. Returns 0 if it fits in `size` bytes (and writes it), non-zero if not.
 */
int results_csv_default_path(const char* dir, time_t when, char* path, size_t size);

/**
 * Creates the file at `path` (which must not exist) and writes the header for roads of `length_h` and
 * `length_v` cells. On success, *csv must be released with results_csv_close; *csv is untouched on
 * failure.
 */
results_csv_status_t results_csv_open(const char* path, unsigned int length_h, unsigned int length_v, results_csv_t** csv);

/** Appends a row and flushes it. */
results_csv_status_t results_csv_write(results_csv_t* csv, const results_row_t* row);

/** Closes the file and sets *csv to NULL. Returns EIO if the last bytes could not be written. */
results_csv_status_t results_csv_close(results_csv_t** csv);

#endif
