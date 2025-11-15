#ifndef LIST_H
#define LIST_H

#include "car.h"

#define LIST_MAX_SIZE 100

typedef struct list_t {
    car_t* items[LIST_MAX_SIZE];
    int size;
} list_t;

void list_init(list_t* list);

void list_append(list_t* list, car_t* car);
void list_remove(list_t* list, int index);
void list_destroy(list_t* list);

car_t* list_get(list_t* list, int index);

#endif
