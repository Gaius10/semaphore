#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include "./input.h"

struct Input {
    FILE* in;
    bool is_terminal;
    struct termios original;
};

input_status_t input_create(FILE* in, input_t** input) {
    input_t* new_input = malloc(sizeof(input_t));
    if (new_input == NULL) {
        return INPUT_STATUS_ENOMEM;
    }

    new_input->in = in;
    new_input->is_terminal = tcgetattr(fileno(in), &new_input->original) == 0;

    if (new_input->is_terminal) {
        struct termios raw = new_input->original;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG);
        // A read waits for 1 byte, but gives up after 0.1s (VTIME is in tenths of a second).
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;
        tcsetattr(fileno(in), TCSANOW, &raw);
    }

    *input = new_input;

    return INPUT_STATUS_OK;
}

void input_destroy(input_t** input) {
    if (!input || !*input) {
        return;
    }

    if ((*input)->is_terminal) {
        tcsetattr(fileno((*input)->in), TCSANOW, &(*input)->original);
    }

    free(*input);
    *input = NULL;
}

input_action_t input_read(input_t* input) {
    int key = getc(input->in);

    if (key == EOF) {
        // On a terminal, EOF only means that the 0.1s ran out. The flag sticks, so clear it.
        if (input->is_terminal) {
            clearerr(input->in);
            return INPUT_ACTION_NONE;
        }
        return INPUT_ACTION_QUIT;
    }

    switch (key) {
        case 'h': return INPUT_ACTION_HORIZONTAL_TOGGLE;
        case 'v': return INPUT_ACTION_VERTICAL_TOGGLE;
        case 'q':
        case 0x03: /* Ctrl-C */ return INPUT_ACTION_QUIT;
        default: return INPUT_ACTION_NONE;
    }
}
