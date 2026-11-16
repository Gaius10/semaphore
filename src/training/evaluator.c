#define _DEFAULT_SOURCE /* MAP_ANONYMOUS, fork, waitpid and nanosleep are hidden under strict -std=c2x */

#include "evaluator.h"

#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PROGRESS_INTERVAL_NS 50000000L /* 50 ms between progress calls */
#define POLL_INTERVAL_NS 1000000L      /* 1 ms between checks on the workers */

struct evaluator {
    const config_t* config;
    evaluator_play_fn play;
    unsigned int genome_length;
    unsigned int capacity;
    unsigned int workers;
    unsigned int scored; /* how many candidates the last population had */

    evaluator_progress_fn progress;
    void* progress_context;

    /* One block of shared memory: the workers write their results and bump their counters in it. */
    void* mapping;
    size_t mapping_size;
    candidate_result_t* results; /* capacity of them */
    atomic_uint* finished;       /* one per worker: candidates it is done with */

    /* Parent-only scratch for the progress callback and the worker bookkeeping. */
    unsigned int* assigned;
    unsigned int* finished_copy;
    pid_t* pids;
    pid_t parent; /* the process that forks the workers */
};

/** The work of one worker: plays its candidates and exits. Never returns. */
static void worker_main(
    evaluator_t* self,
    unsigned int worker,
    const double* genomes,
    unsigned int count,
    unsigned int generation
);
static bool wait_for_workers(evaluator_t* self, unsigned int generation, unsigned int count);
static bool poll_workers(evaluator_t* self, unsigned int* alive);
static void stop_workers(evaluator_t* self);
static void report_progress(evaluator_t* self, unsigned int generation, unsigned int count);
static long nanoseconds_since(const struct timespec* start);

evaluator_status_t evaluator_create(const config_t* config, unsigned int capacity, evaluator_t** evaluator) {
    if (config == NULL) {
        return EVALUATOR_STATUS_EINVAL;
    }

    return evaluator_create_with(config, capacity, config_get_training(config).workers, candidate_play, evaluator);
}

evaluator_status_t evaluator_create_with(
    const config_t* config,
    unsigned int capacity,
    unsigned int workers,
    evaluator_play_fn play,
    evaluator_t** evaluator
) {
    if (config == NULL || capacity == 0 || workers == 0 || play == NULL || evaluator == NULL) {
        return EVALUATOR_STATUS_EINVAL;
    }

    evaluator_t* self = calloc(1, sizeof(evaluator_t));
    if (self == NULL) {
        return EVALUATOR_STATUS_ENOMEM;
    }

    self->mapping_size = capacity * sizeof(candidate_result_t) + workers * sizeof(atomic_uint);
    self->mapping = mmap(NULL, self->mapping_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    self->assigned = calloc(workers, sizeof(unsigned int));
    self->finished_copy = calloc(workers, sizeof(unsigned int));
    self->pids = calloc(workers, sizeof(pid_t));
    if (self->mapping == MAP_FAILED || self->assigned == NULL || self->finished_copy == NULL || self->pids == NULL) {
        if (self->mapping != MAP_FAILED) {
            munmap(self->mapping, self->mapping_size);
        }
        free(self->assigned);
        free(self->finished_copy);
        free(self->pids);
        free(self);
        return EVALUATOR_STATUS_ENOMEM;
    }

    /* Results first (8-byte aligned), then the counters. The mapping starts zeroed. */
    self->results = self->mapping;
    self->finished = (atomic_uint*)(self->results + capacity);

    self->config = config;
    self->play = play;
    self->genome_length = candidate_genome_length(config);
    self->capacity = capacity;
    self->workers = workers;

    *evaluator = self;
    return EVALUATOR_STATUS_OK;
}

void evaluator_destroy(evaluator_t** evaluator) {
    if (evaluator == NULL || *evaluator == NULL) {
        return;
    }

    evaluator_t* self = *evaluator;
    munmap(self->mapping, self->mapping_size);
    free(self->assigned);
    free(self->finished_copy);
    free(self->pids);
    free(self);
    *evaluator = NULL;
}

void evaluator_set_progress(evaluator_t* evaluator, evaluator_progress_fn progress, void* context) {
    evaluator->progress = progress;
    evaluator->progress_context = context;
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
    self->parent = getpid();
    unsigned int started = count < self->workers ? count : self->workers; /* no worker without candidates */
    for (unsigned int w = 0; w < self->workers; w++) {
        atomic_store(&self->finished[w], 0);
        self->assigned[w] = w < started ? (count - w + started - 1) / started : 0;
        self->pids[w] = 0;
    }

    for (unsigned int w = 0; w < started; w++) {
        /* Children copy the genomes (and everything else) as they are at this moment: nothing to send them. */
        pid_t pid = fork();
        if (pid < 0) {
            stop_workers(self);
            return 1;
        }

        if (pid == 0) {
            worker_main(self, w, genomes, count, generation);
        }

        self->pids[w] = pid;
    }

    if (!wait_for_workers(self, generation, count)) {
        stop_workers(self);
        return 1;
    }

    for (unsigned int i = 0; i < count; i++) {
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

/**
 * _exit and not exit: the child holds a copy of the parent's buffered output and of its exit handlers,
 * and none of that may run a second time.
 */
static void worker_main(
    evaluator_t* self,
    unsigned int worker,
    const double* genomes,
    unsigned int count,
    unsigned int generation
) {
    unsigned int started = count < self->workers ? count : self->workers;

    /*
     * Ctrl-C goes to the whole process group, workers included. The caller decides how to stop (the
     * training waits for the generation to finish), so a worker ignores it...
     */
    signal(SIGINT, SIG_IGN);
#ifdef __linux__
    /* ...but a worker must not outlive the process that is waiting for it. */
    prctl(PR_SET_PDEATHSIG, SIGKILL);
    if (getppid() != self->parent) {
        _exit(1); /* the parent was already gone before the line above took effect */
    }
#endif

    for (unsigned int i = worker; i < count; i += started) {
        const double* genome = &genomes[(size_t)i * self->genome_length];
        if (self->play(self->config, genome, generation, &self->results[i]) != CANDIDATE_STATUS_OK) {
            _exit(1);
        }

        atomic_fetch_add(&self->finished[worker], 1);
    }

    _exit(0);
}

/** Waits until every worker has exited cleanly. Returns false as soon as one fails. */
static bool wait_for_workers(evaluator_t* self, unsigned int generation, unsigned int count) {
    struct timespec last_report;
    struct timespec poll = { .tv_sec = 0, .tv_nsec = POLL_INTERVAL_NS };

    report_progress(self, generation, count);
    clock_gettime(CLOCK_MONOTONIC, &last_report);

    unsigned int alive = 0;
    do {
        if (!poll_workers(self, &alive)) {
            return false;
        }

        if (alive > 0) {
            if (nanoseconds_since(&last_report) >= PROGRESS_INTERVAL_NS) {
                report_progress(self, generation, count);
                clock_gettime(CLOCK_MONOTONIC, &last_report);
            }
            nanosleep(&poll, NULL);
        }
    } while (alive > 0);

    report_progress(self, generation, count);
    return true;
}

/** Reaps the workers that have exited. Returns false if one of them failed or died. */
static bool poll_workers(evaluator_t* self, unsigned int* alive) {
    *alive = 0;

    for (unsigned int w = 0; w < self->workers; w++) {
        if (self->pids[w] <= 0) {
            continue;
        }

        int status = 0;
        pid_t done = waitpid(self->pids[w], &status, WNOHANG);
        if (done == 0 || (done < 0 && errno == EINTR)) {
            (*alive)++;
            continue;
        }

        self->pids[w] = 0;
        if (done < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return false;
        }
    }

    return true;
}

/** Kills whatever is still running and reaps every worker, so that none is left behind. */
static void stop_workers(evaluator_t* self) {
    for (unsigned int w = 0; w < self->workers; w++) {
        if (self->pids[w] <= 0) {
            continue;
        }

        kill(self->pids[w], SIGKILL);
        while (waitpid(self->pids[w], NULL, 0) < 0 && errno == EINTR) {
        }
        self->pids[w] = 0;
    }
}

static void report_progress(evaluator_t* self, unsigned int generation, unsigned int count) {
    if (self->progress == NULL) {
        return;
    }

    unsigned int done = 0;
    for (unsigned int w = 0; w < self->workers; w++) {
        self->finished_copy[w] = atomic_load(&self->finished[w]);
        done += self->finished_copy[w];
    }

    evaluator_progress_t progress = {
        .generation = generation,
        .total = count,
        .done = done,
        .workers = self->workers,
        .assigned = self->assigned,
        .finished = self->finished_copy,
    };
    self->progress(&progress, self->progress_context);
}

static long nanoseconds_since(const struct timespec* start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return (long)(now.tv_sec - start->tv_sec) * 1000000000L + (now.tv_nsec - start->tv_nsec);
}
