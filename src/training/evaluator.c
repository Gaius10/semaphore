#include "evaluator.h"

#include <stdlib.h>

struct evaluator {
    const config_t* config;
    unsigned int genome_length;
    unsigned int capacity;
    unsigned int scored; /* how many candidates the last population had */
    candidate_result_t* results;
};

evaluator_status_t evaluator_create(const config_t* config, unsigned int capacity, evaluator_t** evaluator) {
    if (config == NULL || capacity == 0 || evaluator == NULL) {
        return EVALUATOR_STATUS_EINVAL;
    }

    evaluator_t* self = calloc(1, sizeof(evaluator_t));
    if (self == NULL) {
        return EVALUATOR_STATUS_ENOMEM;
    }

    self->results = calloc(capacity, sizeof(candidate_result_t));
    if (self->results == NULL) {
        free(self);
        return EVALUATOR_STATUS_ENOMEM;
    }

    self->config = config;
    self->genome_length = candidate_genome_length(config);
    self->capacity = capacity;

    *evaluator = self;
    return EVALUATOR_STATUS_OK;
}

void evaluator_destroy(evaluator_t** evaluator) {
    if (evaluator == NULL || *evaluator == NULL) {
        return;
    }

    free((*evaluator)->results);
    free(*evaluator);
    *evaluator = NULL;
}

int evaluator_evaluate(
    const double* genomes,
    unsigned int count,
    unsigned int genome_length,
    unsigned int generation,
    double* fitness_out,
    void* context
) {
    evaluator_t* self = context;
    if (self == NULL || genomes == NULL || fitness_out == NULL) {
        return 1;
    }

    if (genome_length != self->genome_length || count > self->capacity) {
        return 1;
    }

    self->scored = 0;
    for (unsigned int i = 0; i < count; i++) {
        const double* genome = &genomes[(size_t)i * genome_length];
        if (candidate_play(self->config, genome, generation, &self->results[i]) != CANDIDATE_STATUS_OK) {
            return 1;
        }

        fitness_out[i] = self->results[i].fitness;
    }

    self->scored = count;
    return 0;
}

const candidate_result_t* evaluator_result(const evaluator_t* evaluator, unsigned int index) {
    if (evaluator == NULL || index >= evaluator->scored) {
        return NULL;
    }

    return &evaluator->results[index];
}
