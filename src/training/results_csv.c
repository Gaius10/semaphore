#define _POSIX_C_SOURCE 200809L /* localtime_r is hidden under strict -std=c2x */

#include "results_csv.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

struct results_csv {
    FILE* file;
    unsigned int genome_length;
};

int results_csv_default_path(const char* dir, time_t when, char* path, size_t size) {
    struct tm local;
    char stamp[32];

    if (localtime_r(&when, &local) == NULL || strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &local) == 0) {
        return 1;
    }

    int written = snprintf(path, size, "%s/training_%s.csv", dir, stamp);
    return written < 0 || (size_t)written >= size;
}

results_csv_status_t results_csv_open(const char* path, unsigned int length_h, unsigned int length_v, results_csv_t** csv) {
    results_csv_t* self = malloc(sizeof(results_csv_t));
    if (self == NULL) {
        return RESULTS_CSV_STATUS_ENOMEM;
    }

    self->file = fopen(path, "wx"); // "x": fail if the file exists, so no run ever overwrites another
    if (self->file == NULL) {
        results_csv_status_t status = errno == EEXIST ? RESULTS_CSV_STATUS_EEXIST : RESULTS_CSV_STATUS_EIO;
        free(self);
        return status;
    }

    self->genome_length = 1 + length_h + length_v;

    fputs(
        "generation,champion_fitness,mean_fitness,worst_fitness,"
        "champion_cars_passed,champion_crashes,threshold",
        self->file
    );
    for (unsigned int i = 0; i < length_h; i++) {
        fprintf(self->file, ",h%u", i);
    }
    for (unsigned int i = 0; i < length_v; i++) {
        fprintf(self->file, ",v%u", i);
    }
    fputc('\n', self->file);

    if (fflush(self->file) != 0) {
        fclose(self->file);
        free(self);
        return RESULTS_CSV_STATUS_EIO;
    }

    *csv = self;
    return RESULTS_CSV_STATUS_OK;
}

results_csv_status_t results_csv_write(results_csv_t* csv, const results_row_t* row) {
    fprintf(
        csv->file,
        "%u,%.17g,%.17g,%.17g,%.17g,%.17g",
        row->generation,
        row->champion_fitness,
        row->mean_fitness,
        row->worst_fitness,
        row->champion_cars_passed,
        row->champion_crashes
    );
    for (unsigned int g = 0; g < csv->genome_length; g++) {
        fprintf(csv->file, ",%.17g", row->genome[g]);
    }
    fputc('\n', csv->file);

    if (fflush(csv->file) != 0 || ferror(csv->file)) {
        return RESULTS_CSV_STATUS_EIO;
    }

    return RESULTS_CSV_STATUS_OK;
}

results_csv_status_t results_csv_close(results_csv_t** csv) {
    if (csv == NULL || *csv == NULL) {
        return RESULTS_CSV_STATUS_OK;
    }

    results_csv_status_t status = fclose((*csv)->file) == 0 ? RESULTS_CSV_STATUS_OK : RESULTS_CSV_STATUS_EIO;
    free(*csv);
    *csv = NULL;
    return status;
}
