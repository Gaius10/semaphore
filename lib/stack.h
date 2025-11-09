
#ifndef STACK_H
#define STACK_H

#define STACK_MAX_SIZE 1000

typedef struct stack Stack;

Stack* create_stack(void);

int is_empty(const Stack *s);
int is_full(const Stack *s);

int push(Stack *s, int value);
int pop(Stack *s, int *value);
int peek(const Stack *s, int *value);

void free_stack(Stack **s);
void print_stack(const Stack *s);

#endif

