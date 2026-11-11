#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

/**
 * Keyboard input: reads single key presses, without waiting for Enter. It only knows keys, not
 * the game, so whoever uses it decides what each action means. It is not thread-safe: use one
 * input from one thread at a time.
 */

typedef struct Input input_t;

/** What the key pressed means. */
typedef enum input_action {
    /** No key was pressed in time (see input_read), or the key is not bound to anything. */
    INPUT_ACTION_NONE,
    /** Key 'h'. */
    INPUT_ACTION_HORIZONTAL_TOGGLE,
    /** Key 'v'. */
    INPUT_ACTION_VERTICAL_TOGGLE,
    /** Key 'q', Ctrl-C, or the end of a file or pipe. */
    INPUT_ACTION_QUIT,
} input_action_t;

typedef enum input_status {
    /** The operation completed successfully. */
    INPUT_STATUS_OK,
    /** Memory allocation failed (input_create). */
    INPUT_STATUS_ENOMEM,
} input_status_t;

/**
 * On success, *input holds a new input that reads from `in` (e.g. stdin) and must be released
 * with input_destroy. If `in` is a terminal, it is put in raw mode (no echo, no line buffering,
 * and Ctrl-C is read as a key) until input_destroy. The input does not own `in`: it never closes it.
 */
input_status_t input_create(FILE* in, input_t** input);
/** Restores the terminal to how it was before input_create. */
void input_destroy(input_t** input);

/**
 * Waits for a key and returns its action. On a terminal it waits at most 0.1 seconds and then
 * returns INPUT_ACTION_NONE, so the caller can check whether it should stop. On a file or pipe it
 * waits as long as needed.
 */
input_action_t input_read(input_t* input);

#endif
