#include <stdlib.h>
#include <stdio.h>
#include "../lib/stack.h"

struct stack {
    int items[STACK_MAX_SIZE];
    unsigned len;
};

Stack* create_stack(void) {
    Stack* s = malloc(sizeof(Stack));
    if (s == NULL) {
        return NULL;
    }

    s->len = 0;
    return s;
}

int _is_empty(const Stack *s) {
    return s->len == 0;
}

int is_empty(const Stack *s) {
    if (s == NULL) {
        return -1;
    }

    return _is_empty(s);
}

int _is_full(const Stack *s) {
    return s->len == STACK_MAX_SIZE;
}

int is_full(const Stack *s) {
    if (s == NULL) {
        return -1;
    }

    return _is_full(s);
}

int push(Stack *s, int value) {
    if (s == NULL) {
        return -1;
    }

    if (_is_full(s)) {
        return -1;
    }

    s->items[s->len] = value;
    s->len++;

    return 1;
}

int pop(Stack *s, int *value) {
    if (s == NULL || value == NULL) {
        return -1;
    }

    if (_is_empty(s)) {
        return -1;
    }

    *value = s->items[s->len - 1];
    s->len--;

    return 1;
}

int peek(const Stack *s, int *value) {
    if (s == NULL || value == NULL) {
        return -1;
    }

    if (s->len == 0) {
        return -1;
    }

    *value = s->items[s->len];

    return 1;
}

void free_stack(Stack **s) {
    free(*s);
    *s = NULL;
}

void print_stack(const Stack *s) {
    for (unsigned i = 0; i < s->len; i++) {
        printf("%d ", s->items[i]);
    }
    printf("\n");
}
