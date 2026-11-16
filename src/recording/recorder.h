#ifndef RECORDER_H
#define RECORDER_H

#include <stddef.h>
#include <time.h>

#include "../config/config.h"
#include "../game/game.h"

/**
 * Records a game, tick by tick, to a JSON Lines file that analysis/replay.html plays back.
 *
 * The first line is a header with the world the game was played in:
 *   {"format":"semaphore-recording","version":1,"mode":"ai","agent":"linear_combination",
 *    "road_length_h":10,"road_length_v":10,"car_spawn_probability_h":0.5,
 *    "car_spawn_probability_v":0.5,"tick_ms":200}
 * ("agent" is null in human mode). Every other line is the state of the game after one tick:
 *   {"t":3,"lh":"g","lv":"r","h":[[1,2],[3,0]],"v":[[2,1]],"passed":0,"crashes":0}
 * t is the tick (0 is the game before its first tick), lh and lv the color of each light ("g" or
 * "r"), h and v the cars of each road as [id, position along the road] (x for the horizontal
 * road, y for the vertical one; the other coordinate is fixed), front car first, and passed and
 * crashes the game's totals so far.
 *
 * Every line is flushed as soon as it is written, so a game stopped with Ctrl-C still leaves a
 * file that plays back up to its last tick.
 */
typedef struct recorder recorder_t;

typedef enum recorder_status {
    RECORDER_STATUS_OK,
    RECORDER_STATUS_ENOMEM, // Memory allocation failed (recorder_open).
    RECORDER_STATUS_EEXIST, // The file already exists; recordings are never overwritten (recorder_open).
    RECORDER_STATUS_EIO,    // The folder or the file could not be created or written.
} recorder_status_t;

/** Writes "<dir>/game_YYYYMMDD_HHMMSS.jsonl" (local time `when`) into path. Returns 0 on success, 1 if it does not fit. */
int recorder_default_path(const char* dir, time_t when, char* path, size_t size);

/**
 * Creates the file at `path` (its folder must exist) and writes the header, taken from `config`
 * (its world, tick and, in ai mode, agent). On success, *recorder must be released with
 * recorder_close.
 */
recorder_status_t recorder_open(const char* path, const config_t* config, recorder_t** recorder);

/**
 * Creates `dir` if it is missing and opens a new recording in it, named by recorder_default_path
 * with the current time. The path used is copied into `path`.
 */
recorder_status_t recorder_open_in(const char* dir, const config_t* config, char* path, size_t size, recorder_t** recorder);

/** Writes the state of `game` as the line of tick `tick`. */
recorder_status_t recorder_write_tick(recorder_t* recorder, unsigned int tick, const game_t* game);

/** Closes the file and frees the recorder. Does nothing if *recorder is NULL. */
void recorder_close(recorder_t** recorder);

#endif
